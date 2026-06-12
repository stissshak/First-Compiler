// MPL/src/AstAnalyser.cpp

#include <unordered_map>
#include <stack>
#include <optional>

#include "AstAnalyser.hpp"

struct varInfo{
    Type* varType;
    bool inited;
    bool isConst = false;
};


struct funcInfo{
    FuncType* funcType;
    bool isDefined;
    bool isExtern = false;
};

struct strctInfo{
    std::vector<std::pair<std::string_view, DeclInfo>> fields;
};

struct DeclInfo{
    std::variant<varInfo, funcInfo, strctInfo> info;
};

struct Scope{
    Scope *parent;
    std::unordered_map<std::string_view, DeclInfo> symbols;
};

strctInfo* lookupStruct(std::string_view name, Scope* curScope);

static bool isByte(Type* t){
    auto b = dynamic_cast<BuiltinType*>(t);
    return b && b->type == BuiltinTypes::Byte;
}

static varInfo* lookupVar(Scope* s, std::string_view name){
    for(; s; s = s->parent){
        auto it = s->symbols.find(name);
        if(it == s->symbols.end()) continue;
        return std::get_if<varInfo>(&it->second.info);
    }
    return nullptr;
}

static void markInited(Scope* s, std::string_view name){
    if(auto v = lookupVar(s, name)) v->inited = true;
}

static bool isFuncName(Scope* s, std::string_view name){
    for(; s; s = s->parent){
        auto it = s->symbols.find(name);
        if(it == s->symbols.end()) continue;
        return std::holds_alternative<funcInfo>(it->second.info);
    }
    return false;
}

void AstAnalyser::err(std::string_view msg){
    ++errCount;
    cl.error("error: " + std::string(msg));
}

void AstAnalyser::err(const Node& n, std::string_view msg){
    ++errCount;
    cl.error(smap.where(n.offset, buffer) + ": error: " + std::string(msg));
}

void AstAnalyser::warn(const Node& n, std::string_view msg){
    cl.warning(smap.where(n.offset, buffer) + ": warning: " + std::string(msg));
}

void AstAnalyser::checkTypes(Type* a, Type* b, const Node& n, std::string_view msg){
    if (!a || !b) return;
    switch (checkCast(a, b)) {
        case castResult::Equal:
        case castResult::Implicit:
            break;
        case castResult::Warn:
            warn(n, msg);
            break;
        case castResult::No:
            err(n, msg);
            break;
    }
}


bool AstAnalyser::analyse(TranslationUnit& unit){
    unit.accept(*this);
    return errCount == 0;
}


// libc-backed builtins, usable without declaration (specs/semantics.md §8)
void AstAnalyser::addBuiltins(){
    auto add = [&](std::string_view name, std::unique_ptr<FuncType> f){
        funcInfo fi{f.get()};
        fi.isExtern = true;   // never defined in MPL, redefinition = error
        funcTypes.push_back(std::move(f));
        curScope->symbols.emplace(name, DeclInfo{fi});
    };
    auto sig = [&](Type& ret, Type* param, bool variadic){
        auto f = std::make_unique<FuncType>();
        f->returnType = ret.clone();
        if(param) f->params.push_back(param->clone());
        f->variadic = variadic;
        return f;
    };

    PointerType charPtr(charType.clone());
    PointerType voidPtr(voidType.clone());

    add("print",  sig(intType,  &charPtr, true));    // printf
    add("input",  sig(intType,  &charPtr, true));    // scanf
    add("exit",   sig(voidType, &intType, false));
    add("panic",  sig(voidType, &charPtr, false));   // message + exit(1)
    add("assert", sig(voidType, &boolType, false));  // runtime error if false
    add("free",   sig(voidType, &voidPtr, false));

    auto m = std::make_unique<FuncType>();           // void* malloc(int)
    m->returnType = voidPtr.clone();
    m->params.push_back(intType.clone());
    add("malloc", std::move(m));
}

void AstAnalyser::visit(TranslationUnit& node){
    curScope = new Scope;
    curScope->parent = nullptr;
    addBuiltins();
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
    auto it = curScope->symbols.find("main");
    if(it == curScope->symbols.end())
        err("no 'main' function");
    else if(auto f = std::get_if<funcInfo>(&it->second.info)){
        auto rt = dynamic_cast<BuiltinType*>(f->funcType->returnType.get());
        if(!rt || rt->type != BuiltinTypes::Int)
            err("'main' must return int");
    }else{
        err("'main' is not a function");
    }

    // only extern funcs may stay without a body
    for(auto& [name, di] : curScope->symbols){
        if(auto f = std::get_if<funcInfo>(&di.info))
            if(!f->isDefined && !f->isExtern)
                err("Function '" + std::string(name) + "' is declared but never defined");
    }
    delete curScope;
}
void AstAnalyser::visit(VarDecl& node){
    if(curScope->symbols.find(node.name) != curScope->symbols.end()){
        err(node, "Var was declarated");
        return;
    }

    if(node.isConst && !node.init && node.initList.empty()){
        err(node, "Const var must be initialized");
        return;
    }

    auto bt = dynamic_cast<BuiltinType*>(node.type.get());
    bool inited = node.init != nullptr
        || dynamic_cast<ArrayType*>(node.type.get())
        || (bt && bt->type == BuiltinTypes::Custom);   // structs fill field by field
    DeclInfo di = {varInfo{node.type.get(), inited, node.isConst}};

    if(node.init){
        node.init->accept(*this);
        checkTypes(node.type.get(), curType, node, "Incompatible types in initialization");
    }

    if(!node.initList.empty()){
        if(auto at = dynamic_cast<ArrayType*>(node.type.get())){
            if(node.initList.size() > at->size) err(node, "Too many initializers");
            for(auto& e : node.initList){
                e->accept(*this);
                checkTypes(at->elemType.get(), curType, *e, "Incompatible type in initializer");
            }
        }
        else if(bt && bt->type == BuiltinTypes::Custom){
            auto si = lookupStruct(bt->name, curScope);
            if(!si) err(node, "Struct type not found");
            else if(node.initList.size() > si->fields.size()) err(node, "Too many initializers");
            else for(std::size_t i = 0; i < node.initList.size(); ++i){
                node.initList[i]->accept(*this);
                checkTypes(std::get<varInfo>(si->fields[i].second.info).varType, curType,
                           *node.initList[i], "Incompatible type in initializer");
            }
        }
        else err(node, "Init list on scalar type");
    }

    curScope->symbols.emplace(node.name, di);
}

void AstAnalyser::visit(StructDecl& node){
    if(curScope->symbols.find(node.name) != curScope->symbols.end()){
        err(node, "Struct was declarated");
        return;
    }

    strctInfo si;

    for(auto& f : node.fields){
       si.fields.emplace_back(f->name, DeclInfo{varInfo{f->type.get(), f->init != nullptr }});
    }
    curScope->symbols.emplace(node.name, DeclInfo(si));
}

void AstAnalyser::analyseFuncBody(FuncDecl& node){
      auto funcScope = new Scope;
      funcScope->parent = curScope;
      curScope = funcScope;

      for(auto& p : node.params)                                                                                                                         
          curScope->symbols.emplace(p->name, DeclInfo{varInfo{p->type.get(), true}});

      auto savedRetType = retType;
      retType = node.returnType.get();
  
      node.body->accept(*this);

      curScope = funcScope->parent;
      retType = savedRetType;
      delete funcScope;
}


// TODO split 2 for
void AstAnalyser::visit(FuncDecl& node){
    auto f = curScope->symbols.find(node.name);
    if(f != curScope->symbols.end()){
        auto existing = std::get_if<funcInfo>(&f->second.info);
        if(!existing || existing->isDefined){
            err(node, "Func was declarated");
            return;
        }
        if(node.body){
            if(existing->isExtern){
                err(node, "Extern function cannot be defined");
                return;
            }
            existing->isDefined = true;
            analyseFuncBody(node);
        }
        return;
    }

    auto ft = std::make_unique<FuncType>();
    ft->returnType = node.returnType->clone();
    ft->variadic = node.variadic;

    for(auto& p: node.params){
        ft->params.push_back(p->type.get()->clone());
    }

    funcInfo fi{ft.get()};
    fi.isExtern = node.isExtern;
    funcTypes.push_back(std::move(ft));

    if(!node.body){
        fi.isDefined = false;
        curScope->symbols.emplace(node.name, DeclInfo(fi));
        return;
    }

    fi.isDefined = true;
    curScope->symbols.emplace(node.name, DeclInfo(fi));

    analyseFuncBody(node);
}


void AstAnalyser::visit(BlockStmt& node){
    auto blockScope = new Scope;
    blockScope->parent = curScope;
    curScope = blockScope;

    for(auto& s: node.statements){
        s->accept(*this);
    }

    curScope = curScope->parent;
    delete blockScope;
}


void AstAnalyser::visit(ExprStmt& node){
    if(node.expr) node.expr->accept(*this);   // null stmt
}

// TODO cond -> bool

void AstAnalyser::visit(IfStmt& node){
    node.cond->accept(*this);
    node.thenPart->accept(*this);
    if(node.elsePart) node.elsePart->accept(*this);
}


void AstAnalyser::visit(WhileStmt& node){
    auto wasInLoop = isInLoop;
    isInLoop = true;
    node.cond->accept(*this);
    node.body->accept(*this);
    isInLoop = wasInLoop;
}


void AstAnalyser::visit(ForStmt& node){
    auto wasInLoop = isInLoop;
    isInLoop = true;
    auto forScope = new Scope;
    forScope->parent = curScope;
    curScope = forScope;

    if(node.initDecl != nullptr) node.initDecl->accept(*this);
    else if(node.initStmt != nullptr) node.initStmt->accept(*this);
    if(node.cond != nullptr) node.cond->accept(*this);
    if(node.incr != nullptr) node.incr->accept(*this);
    node.body->accept(*this);

    curScope = curScope->parent;
    delete forScope;
    isInLoop = wasInLoop;
}


void AstAnalyser::visit(ReturnStmt& node){
    if(node.value) node.value->accept(*this);
    else curType = &voidType;
    checkTypes(retType, curType, node, "Return type mismatch");
}


void AstAnalyser::visit(BreakStmt& node){
    if(!isInLoop){
        err(node, "Break statement not at loop");
    }
}


void AstAnalyser::visit(ContinueStmt& node){
    if(!isInLoop){
        err(node, "Continue statement not at loop");
    }
}


void AstAnalyser::visit(DeclStmt& node){
    node.decl->accept(*this);
}

// Change assign, from binary to other AST node
void AstAnalyser::visit(BinaryExpr& node){
    if(node.op >= BinaryOp::Assign){
        if(node.op > BinaryOp::MinusAssign)   // TODO *= /= %= and bit-assigns
            err(node, "This compound assign is not supported yet");
        if(auto id = dynamic_cast<Identifier*>(node.left.get())){
            if(auto v = lookupVar(curScope, id->name); v && v->isConst)
                err(node, "Assign to const var");
            else if(!v && isFuncName(curScope, id->name))
                err(node, "Function is not assignable");
            // before visiting left, otherwise the target itself errors
            if(node.op == BinaryOp::Assign) markInited(curScope, id->name);
        }
    }

    node.left->accept(*this);
    auto lType = curType;
    node.right->accept(*this);
    auto rType = curType;

    // raw data: only =, ==, != are allowed
    if((isByte(lType) || isByte(rType))
            && node.op != BinaryOp::Assign
            && node.op != BinaryOp::Equal && node.op != BinaryOp::NotEqual)
        err(node, "byte is not arithmetic");

    if(node.op >= BinaryOp::Assign){
        // no stores through const int*
        if(auto u = dynamic_cast<UnaryExpr*>(node.left.get()); u && u->op == UnaryOp::Deref){
            if(auto p = dynamic_cast<PointerType*>(u->child->resultType.get()); p && p->constBase)
                err(node, "Assign through const pointer");
        }
        if(auto ix = dynamic_cast<IndexExpr*>(node.left.get())){
            if(auto p = dynamic_cast<PointerType*>(ix->arr->resultType.get()); p && p->constBase)
                err(node, "Assign through const pointer");
        }
    }

    checkTypes(lType, rType, node, "Incompatible types in binary expression");


    switch(node.op){
        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
        case BinaryOp::LessEqual:
        case BinaryOp::GreaterEqual:
        case BinaryOp::And:
        case BinaryOp::Or:
            curType = &boolType;
            break;
        default:
        // TODO common type
            curType = lType;
            break;
    }
    node.resultType = curType->clone();

}

// TODO portfix + assign check
// TODO weakptr
void AstAnalyser::visit(UnaryExpr& node){
    // &x lets scanf and others init through the pointer
    if(node.op == UnaryOp::AddressOf){
        if(auto id = dynamic_cast<Identifier*>(node.child.get())){
            markInited(curScope, id->name);
        }
    }

    node.child->accept(*this);

    if(isByte(curType) && node.op != UnaryOp::AddressOf)
        err(node, "byte is not arithmetic");

    switch(node.op){
        case UnaryOp::Pos:
        case UnaryOp::Neg:
        case UnaryOp::PreInc:
        case UnaryOp::PreDec:
        case UnaryOp::PostInc:
        case UnaryOp::PostDec:
            break;
        case UnaryOp::Not:
            curType = &intType;
            break;
        case UnaryOp::AddressOf:{
            if(curType == nullptr){
                err(node, "Cannot take address of unknown type");
                break;
            }
            auto pt = std::make_unique<PointerType>();
            pt->base = curType->clone();
            // &const-var gives const int*
            if(auto id = dynamic_cast<Identifier*>(node.child.get())){
                if(auto v = lookupVar(curScope, id->name); v && v->isConst)
                    pt->constBase = true;
            }
            curType = pt.get();
            ptrTypes.push_back(std::move(pt));
            break;
        }
        case UnaryOp::Deref:
        // TODO void pointer deref
        if(auto pt = dynamic_cast<PointerType*>(curType)){
            curType = pt->base.get();
            if(auto tmp = dynamic_cast<BuiltinType*>(curType); tmp && tmp->type == BuiltinTypes::Void){
                err(node, "Deref of void pointer");
            }
        } else {
            err(node, "Deref of not a pointer");
        }
        break;

    }
    if(curType) node.resultType = curType->clone();
}


void AstAnalyser::visit(CallExpr& node){
    node.func->accept(*this);
    auto fType = dynamic_cast<FuncType*>(curType);
    if(fType == nullptr){
        err(node, "Called object is not a function");
        curType = nullptr;
        return;
    }
    else{
        bool countOk = fType->variadic ? node.param.size() >= fType->params.size()
                                       : node.param.size() == fType->params.size();
        if(!countOk){
            err(node, "Declarated number of arguments not equal");
        }
        else{
            for(std::size_t i = 0; i < node.param.size(); ++i){
                node.param[i]->accept(*this);
                if(i < fType->params.size())
                    checkTypes(fType->params[i].get(), curType, *node.param[i], "Incompatible argument type");
            }
            curType = fType->returnType.get();
        }
    }
    node.resultType = curType->clone();
}

void AstAnalyser::visit(CastExpr& node){
    node.expr->accept(*this);
    // byte converts only by explicit cast, and only with int/char
    bool byteCast = (isByte(node.target.get()) || isByte(curType))
        && typeIndex(node.target.get()) != -1 && typeIndex(curType) != -1
        && typeIndex(node.target.get()) != 1 && typeIndex(curType) != 1;   // not float
    if(!byteCast)
        checkTypes(node.target.get(), curType, node, "Incompatible cast types");
    curType = node.target.get();
    node.resultType = curType->clone();
}

void AstAnalyser::visit(IndexExpr& node){
    node.index->accept(*this);
    checkTypes(curType, &intType, node, "Must be integer in Array");
    node.arr->accept(*this);
    if(auto at = dynamic_cast<ArrayType*>(curType)){
        curType = at->elemType.get();
    }
    else if(auto pt = dynamic_cast<PointerType*>(curType)) curType = pt->base.get();
    else{
        err(node, "Not Array");
    }
    node.resultType = curType->clone();
}

strctInfo* lookupStruct(std::string_view name, Scope* curScope){
    for (Scope* s = curScope; s; s = s->parent){
        auto it = s->symbols.find(name);
        if (it == s->symbols.end()) continue;
        return std::get_if<strctInfo>(&it->second.info);
    }
    return nullptr;
}

void AstAnalyser::visit(AccessExpr& node){
    node.object->accept(*this);

    BuiltinType* bt = nullptr;
    if(node.kind == AccessKind::Dot){
        bt = dynamic_cast<BuiltinType*>(curType);
        if(!bt || bt->type != BuiltinTypes::Custom){
            err(node, "Member access on non-struct type");
            return;
        }
    }else{
        auto pt = dynamic_cast<PointerType*>(curType);
        if(!pt){
            err(node, "Arrow on non-pointer");
            return;
        }
        bt = dynamic_cast<BuiltinType*>(pt->base.get());
        if(!bt || bt->type != BuiltinTypes::Custom){
            err(node, "Member access on non-struct type");
            return;
        }
    }

    auto si = lookupStruct(bt->name, curScope);
    if(!si){
        err(node, "Struct type not found");
        return;
    }

    for(auto& f: si->fields){
        if(f.first == node.field){
            curType = std::get<varInfo>(f.second.info).varType;
            node.resultType = curType->clone();
            return;
        }
    }
    err(node, "Field not found in struct");
}

void AstAnalyser::visit(SizeofExpr& node){
    if(node.expr) node.expr->accept(*this);   // only for the type, not evaluated
    curType = &intType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(TypeidExpr& node){
    node.expr->accept(*this);
    auto pt = std::make_unique<PointerType>(charType.clone());
    curType = pt.get();
    node.resultType = pt->clone();
    ptrTypes.push_back(std::move(pt));
}


void AstAnalyser::visit(IntLiteral& node){
    (void)node;
    curType = &intType;
    node.resultType = curType->clone();
}


void AstAnalyser::visit(FloatLiteral& node){
    (void)node;
    curType = &floatType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(CharLiteral& node){
    (void)node;
    curType = &charType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(BoolLiteral& node){
    (void)node;
    curType = &boolType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(StringLiteral& node){
    (void)node;
    auto pt = std::make_unique<PointerType>();
    pt->base = charType.clone();
    curType = pt.get();
    ptrTypes.push_back(std::move(pt));
    node.resultType = curType->clone();
}


void AstAnalyser::visit(Identifier& node){
    for(Scope* s = curScope; s; s = s->parent){
        auto it = s->symbols.find(node.name);
        if(it == s->symbols.end()) continue;
        if(auto v = std::get_if<varInfo>(&it->second.info)){
            curType = v->varType;
            node.resultType = curType->clone();
            if(!v->inited){
                err(node, "Var is not inited");
            }
            return;
        }
        if(auto f = std::get_if<funcInfo>(&it->second.info)){
            curType = f->funcType;
            node.resultType = curType->clone();
            return;
        }
          
        continue;
      }
    err(node, "Identifier not found");
}


void AstAnalyser::visit(BuiltinType& node){
    (void)node;
}


void AstAnalyser::visit(PointerType& node){
    (void)node;
}

void AstAnalyser::visit(FuncType&  node){
    (void)node;
}

void AstAnalyser::visit(ArrayType& node){
    (void)node;
}