// MPL/src/AstAnalyser.cpp

#include <unordered_map>
#include <stack>
#include <optional>

#include "AstAnalyser.hpp"
#include "Logger.hpp"

// TODO 
// StringLiteral/FuncType - MakeType

ConsoleLogger cl;

struct varInfo{
    Type* varType;
    bool inited;
};


struct funcInfo{
    FuncType* funcType;

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

void AstAnalyser::checkTypes(Type* a, Type* b, std::string_view msg){
    if (!a || !b) return;
    switch (checkCast(a, b)) {
        case castResult::Equal:
        case castResult::Implicit:
            break;
        case castResult::Warn:
            cl.warning(msg);
            break;
        case castResult::No:
            cl.error(msg);
            break;
    }
}


void AstAnalyser::analyse(TranslationUnit& unit){
    unit.accept(*this);
}


void AstAnalyser::visit(TranslationUnit& node){
    curScope = new Scope;
    curScope->parent = nullptr;
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
    delete curScope;
}
void AstAnalyser::visit(VarDecl& node){
    DeclInfo di = {varInfo{node.type.get(), node.init != nullptr}};

    if(curScope->symbols.find(node.name) != curScope->symbols.end()){
        cl.error("Var was declarated");
        return;
    }

    if(node.init){
        node.init->accept(*this);
        checkTypes(node.type.get(), curType, "Incompatible types in initialization");
    }

    curScope->symbols.emplace(node.name, di);
}

void AstAnalyser::visit(StructDecl& node){
    strctInfo si;

    for(auto& f : node.fields){
       si.fields.emplace_back(f->name, DeclInfo{varInfo{f->type.get(), f->init != nullptr }});
    }

    if(curScope->symbols.find(node.name) != curScope->symbols.end()){
        cl.error("Struct was declarated");
        return;
    }
    curScope->symbols.emplace(node.name, DeclInfo(si));
}


void AstAnalyser::visit(FuncDecl& node){
    if(curScope->symbols.find(node.name) != curScope->symbols.end()){
        cl.error("Func was declarated");
        return;
    }

    auto ft = std::make_unique<FuncType>();
    ft->returnType = node.returnType->clone();

    for(auto& p: node.params){
        ft->params.push_back(p->type.get()->clone());
    }

    funcInfo fi{ft.get()};
    funcTypes.push_back(std::move(ft));

    
    curScope->symbols.emplace(node.name, DeclInfo(fi));

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
    node.expr->accept(*this);
}


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
    if(retType == nullptr){
        cl.error("Func not returning anything");
        return;
    }
    if(node.value) node.value->accept(*this);
    else curType = &voidType;
    checkTypes(retType, curType, "Return type mismatch");
}


void AstAnalyser::visit(BreakStmt& node){
    (void)node;
    if(!isInLoop){
        cl.error("Break statement not at loop");
    }
}


void AstAnalyser::visit(ContinueStmt& node){
    (void)node;
    if(!isInLoop){
        cl.error("Continue statement not at loop");
    }
}


void AstAnalyser::visit(DeclStmt& node){
    node.decl->accept(*this);
}


void AstAnalyser::visit(BinaryExpr& node){
    if(node.op == BinaryOp::Assign) {
        if(auto id = dynamic_cast<Identifier*>(node.left.get())){
            for(Scope* s = curScope; s; s = s->parent){
                auto it = s->symbols.find(id->name);
                if(it != s->symbols.end()){
                    if (auto v = std::get_if<varInfo>(&it->second.info)) v->inited = true;
                    break;
                }
            }
        }
    }

    node.left->accept(*this);
    auto lType = curType;
    node.right->accept(*this);
    auto rType = curType;

    checkTypes(lType, rType, "Incompatible types in binary expression");


    switch(node.op){
        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
        case BinaryOp::LessEqual:
        case BinaryOp::GreaterEqual:
        case BinaryOp::And:
        case BinaryOp::Or:
            curType = &intType;
            break;
        default:
            curType = lType;
            break;
    }

}


void AstAnalyser::visit(UnaryExpr& node){
    node.child->accept(*this);

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
                cl.error("Cannot take address of unknown type");
                break;
            }
            auto pt = std::make_unique<PointerType>();
            pt->base = curType->clone();
            curType = pt.get();
            ptrTypes.push_back(std::move(pt));
            break;
        }
        case UnaryOp::Deref:
        if(auto pt = dynamic_cast<PointerType*>(curType)){
            curType = pt->base.get();
        } else {
            cl.error("Deref of not a pointer");
        }
        break;

    }
}


void AstAnalyser::visit(CallExpr& node){
    node.func->accept(*this);
    auto fType = dynamic_cast<FuncType*>(curType);
    if(fType == nullptr){
        cl.error("Called object is not a function");
        curType = nullptr;
        return;
    }
    else{
        if(node.param.size() != fType->params.size()){
            cl.error("Declarated number of arguments not equal");
        }
        else{
            for(std::size_t i = 0; i < node.param.size(); ++i){
                node.param[i]->accept(*this);
                checkTypes(fType->params[i].get(), curType, "Incompatible argument type");

            }
            curType = fType->returnType.get();
        }
    }
}

void AstAnalyser::visit(CastExpr& node){
    node.expr->accept(*this);
    checkTypes(node.target.get(), curType, "Incompatible cast types");
}

void AstAnalyser::visit(IndexExpr& node){
    node.index->accept(*this);
    if(curType != &intType){
        cl.error("Must be integer");
    }

    node.arr->accept(*this);
    if(auto at = dynamic_cast<ArrayType*>(curType)){
        curType = at->elemType.get();
    }
    else if(auto pt = dynamic_cast<PointerType*>(curType)) curType = pt->base.get();

    else{
        cl.error("Not Array");
    }

}

strctInfo* lookupStruct(std::string_view name, Scope* curScope){
    for (Scope* s = curScope; s; s = s->parent){
        auto it = s->symbols.find(name);
        if (it == s->symbols.end()) continue;
        return std::get_if<strctInfo>(&it->second.info);
    }
    return nullptr;
}

// TODO all scopes
void AstAnalyser::visit(AccessExpr& node){
    node.object->accept(*this);

    BuiltinType* bt = nullptr;
    if(node.kind == AccessKind::Dot){
        bt = dynamic_cast<BuiltinType*>(curType);
        if(!bt || bt->type != BuiltinTypes::Custom){
            cl.error("Member access on non-struct type");
            return;
        }
    }else{
        auto pt = dynamic_cast<PointerType*>(curType);
        if(!pt){
            cl.error("Arrow on non-pointer");
            return;
        }
        bt = dynamic_cast<BuiltinType*>(pt->base.get());
        if(!bt || bt->type != BuiltinTypes::Custom){
            cl.error("Member access on non-struct type");
            return;
        }
    }

    auto si = lookupStruct(bt->name, curScope);
    if(!si){
        cl.error("Struct type not found");
        return;
    }

    for(auto& f: si->fields){
        if(f.first == node.field){
            curType = std::get<varInfo>(f.second.info).varType;
            return;
        }
    }
    cl.error("Field not found in struct");
}


void AstAnalyser::visit(IntLiteral& node){
    (void)node;
    curType = &intType;
}


void AstAnalyser::visit(FloatLiteral& node){
    (void)node;
    curType = &floatType;
}

void AstAnalyser::visit(CharLiteral& node){
    (void)node;
    curType = &charType;
}

void AstAnalyser::visit(StringLiteral& node){
    (void)node;
    auto pt = std::make_unique<PointerType>();
    pt->base = charType.clone();
    curType = pt.get();
    ptrTypes.push_back(std::move(pt));
}


void AstAnalyser::visit(Identifier& node){
    for(Scope* s = curScope; s; s = s->parent){
        auto it = s->symbols.find(node.name);
        if(it == s->symbols.end()) continue;
        if(auto v = std::get_if<varInfo>(&it->second.info)){
            curType = v->varType;
            if(!v->inited){
                cl.error("Var is not inited");
            }
            return;
        }
        if(auto f = std::get_if<funcInfo>(&it->second.info)){
            curType = f->funcType;
            return;
        }
          
        continue;
      }
    cl.error("Identifier not found");
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