// MPL/src/AstAnalyser.cpp

#include <cctype>
#include <optional>
#include <stack>
#include <unordered_map>

#include "sema/AstAnalyser.hpp"

struct varInfo {
    Type* varType;
    bool inited;
    bool isConst = false;
};

struct funcOverload {
    FuncType* funcType;
    bool isDefined = false;
    bool isExtern = false;
    std::string mangled; // emitted symbol; empty for builtins (no FuncDecl)
};

struct funcInfo {
    std::vector<funcOverload> overloads; // A.2.8: one name, several signatures
};

struct strctInfo {
    std::vector<std::pair<std::string_view, DeclInfo>> fields;
};

struct DeclInfo {
    std::variant<varInfo, funcInfo, strctInfo> info;
};

struct Scope {
    Scope* parent;
    std::unordered_map<std::string_view, DeclInfo> symbols;
};

strctInfo* lookupStruct(std::string_view name, Scope* curScope);

static bool isByte(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    return b && b->type == BuiltinTypes::Byte;
}

// short code for a type, used to build unique overload symbols
static std::string mangleType(Type* t) {
    if (auto b = dynamic_cast<BuiltinType*>(t)) {
        switch (b->type) {
        case BuiltinTypes::Int:
            return "i";
        case BuiltinTypes::UInt:
            return "j";
        case BuiltinTypes::Short:
            return "s";
        case BuiltinTypes::Long:
            return "l";
        case BuiltinTypes::Float:
            return "f";
        case BuiltinTypes::Char:
            return "c";
        case BuiltinTypes::Bool:
            return "b";
        case BuiltinTypes::Byte:
            return "y";
        case BuiltinTypes::Void:
            return "v";
        case BuiltinTypes::Custom:
            return "S" + std::string(b->name);
        }
    }
    if (auto p = dynamic_cast<PointerType*>(t))
        return "P" + mangleType(p->base.get());
    if (auto a = dynamic_cast<ArrayType*>(t))
        return "A" + mangleType(a->elemType.get());
    if (auto fn = dynamic_cast<FuncType*>(t)) {
        std::string s = "F" + mangleType(fn->returnType.get());
        for (auto& p : fn->params)
            s += mangleType(p.get());
        return s + "E";
    }
    return "X";
}

// emitted symbol for a function. main and externs keep their exact name (C
// linkage); every other function is type-suffixed so overloads don't collide.
static std::string mangleFunc(const FuncDecl& f) {
    if (f.isExtern || f.name == "main")
        return std::string(f.name);
    std::string m = std::string(f.name) + "_";
    if (f.params.empty())
        return m + "v";
    for (auto& p : f.params)
        m += mangleType(p->type.get());
    return m;
}

static bool isFloatTy(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    return b && b->type == BuiltinTypes::Float;
}

static bool isBoolTy(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    return b && b->type == BuiltinTypes::Bool;
}

static bool isScalar(Type* t) {
    auto b = dynamic_cast<BuiltinType*>(t);
    if (!b)
        return false;
    switch (b->type) {
    case BuiltinTypes::Int:
    case BuiltinTypes::Float:
    case BuiltinTypes::Char:
    case BuiltinTypes::Bool:
    case BuiltinTypes::Short:
    case BuiltinTypes::Long:
    case BuiltinTypes::UInt:
        return true;
    default:
        return false;
    }
}

static void coerce(std::unique_ptr<Expr>& e, Type* to) {
    if (!e || !e->resultType || !to)
        return;
    Type* from = e->resultType.get();
    if (!isScalar(from) || !isScalar(to))
        return;
    bool classChange = isFloatTy(from) != isFloatTy(to);
    bool normBool = isBoolTy(to) && !isBoolTy(from);
    if (!classChange && !normBool)
        return;
    auto c = std::make_unique<CastExpr>();
    c->offset = e->offset;
    c->target = to->clone();
    c->resultType = to->clone();
    c->expr = std::move(e);
    e = std::move(c);
}

static varInfo* lookupVar(Scope* s, std::string_view name) {
    for (; s; s = s->parent) {
        auto it = s->symbols.find(name);
        if (it == s->symbols.end())
            continue;
        return std::get_if<varInfo>(&it->second.info);
    }
    return nullptr;
}

static void markInited(Scope* s, std::string_view name) {
    if (auto v = lookupVar(s, name))
        v->inited = true;
}

static bool isFuncName(Scope* s, std::string_view name) {
    for (; s; s = s->parent) {
        auto it = s->symbols.find(name);
        if (it == s->symbols.end())
            continue;
        return std::holds_alternative<funcInfo>(it->second.info);
    }
    return false;
}

void AstAnalyser::err(std::string_view msg) {
    ++errCount;
    cl.error("error: " + std::string(msg));
}

void AstAnalyser::err(const Node& n, std::string_view msg) {
    ++errCount;
    cl.error(smap.where(n.offset, buffer) + ": error: " + std::string(msg));
}

void AstAnalyser::warn(const Node& n, std::string_view msg) {
    cl.warning(smap.where(n.offset, buffer) + ": warning: " + std::string(msg));
}

void AstAnalyser::checkTypes(Type* a, Type* b, const Node& n, std::string_view msg) {
    if (!a || !b)
        return;
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

bool AstAnalyser::analyse(TranslationUnit& unit) {
    unit.accept(*this);
    return errCount == 0;
}

// libc-backed builtins
void AstAnalyser::addBuiltins() {
    auto add = [&](std::string_view name, std::unique_ptr<FuncType> f) {
        funcInfo fi;
        fi.overloads.push_back({f.get(), true, true, ""}); // no FuncDecl, no symbol
        funcTypes.push_back(std::move(f));
        curScope->symbols.emplace(name, DeclInfo{fi});
    };
    auto sig = [&](Type& ret, Type* param, bool variadic) {
        auto f = std::make_unique<FuncType>();
        f->returnType = ret.clone();
        if (param)
            f->params.push_back(param->clone());
        f->variadic = variadic;
        return f;
    };

    PointerType charPtr(charType.clone());
    PointerType voidPtr(voidType.clone());

    add("print", sig(intType, &charPtr, true)); // printf
    add("input", sig(intType, &charPtr, true)); // scanf
    add("exit", sig(voidType, &intType, false));
    add("panic", sig(voidType, &charPtr, false));   // message + exit(1)
    add("assert", sig(voidType, &boolType, false)); // runtime error if false
    add("free", sig(voidType, &voidPtr, false));

    auto m = std::make_unique<FuncType>(); // void* malloc(int)
    m->returnType = voidPtr.clone();
    m->params.push_back(intType.clone());
    add("malloc", std::move(m));
}

void AstAnalyser::visit(TranslationUnit& node) {
    curScope = new Scope;
    curScope->parent = nullptr;
    addBuiltins();
    for (std::size_t i = 0; i < node.decls.size(); ++i) {
        node.decls[i]->accept(*this);
    }
    auto it = curScope->symbols.find("main");
    if (it == curScope->symbols.end())
        err("no 'main' function");
    else if (auto f = std::get_if<funcInfo>(&it->second.info)) {
        auto rt = dynamic_cast<BuiltinType*>(f->overloads[0].funcType->returnType.get());
        if (!rt || rt->type != BuiltinTypes::Int)
            err("'main' must return int");
    } else {
        err("'main' is not a function");
    }

    for (auto& [name, di] : curScope->symbols) {
        if (auto f = std::get_if<funcInfo>(&di.info))
            for (auto& o : f->overloads)
                if (!o.isDefined && !o.isExtern)
                    err("Function '" + std::string(name) + "' is declared but never defined");
    }
    delete curScope;
}
void AstAnalyser::visit(VarDecl& node) {
    if (curScope->symbols.find(node.name) != curScope->symbols.end()) {
        err(node, "Var was declarated");
        return;
    }

    if (node.isConst && !node.init && node.initList.empty()) {
        err(node, "Const var must be initialized");
        return;
    }

    auto bt = dynamic_cast<BuiltinType*>(node.type.get());
    bool inited = node.init != nullptr || dynamic_cast<ArrayType*>(node.type.get()) ||
                  (bt && bt->type == BuiltinTypes::Custom);
    DeclInfo di = {varInfo{node.type.get(), inited, node.isConst}};

    if (node.init) {
        node.init->accept(*this);
        auto at = dynamic_cast<ArrayType*>(node.type.get());
        auto sl = dynamic_cast<StringLiteral*>(node.init.get());
        if (at && sl) { // char[] = "literal"
            auto et = dynamic_cast<BuiltinType*>(at->elemType.get());
            std::size_t len = 0; // decoded length
            auto& v = sl->value;
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (v[i] == '\\' && i + 1 < v.size()) {
                    if (v[++i] == 'x') // \xHH...: consume hex digits
                        while (i + 1 < v.size() && std::isxdigit((unsigned char)v[i + 1]))
                            ++i;
                }
                ++len;
            }
            if (!et || et->type != BuiltinTypes::Char)
                err(node, "String literal initializes only a char array");
            else if (len + 1 > at->size)
                err(node, "String literal too long for array");
        } else {
            checkTypes(node.type.get(), curType, node, "Incompatible types in initialization");
            if (curScope->parent)
                coerce(node.init, node.type.get()); // runtime cvt; globals fold statically
        }
    }

    if (!node.initList.empty()) {
        if (auto at = dynamic_cast<ArrayType*>(node.type.get())) {
            if (node.initList.size() > at->size)
                err(node, "Too many initializers");
            for (auto& e : node.initList) {
                e->accept(*this);
                checkTypes(at->elemType.get(), curType, *e, "Incompatible type in initializer");
                if (curScope->parent)
                    coerce(e, at->elemType.get());
            }
        } else if (bt && bt->type == BuiltinTypes::Custom) {
            auto si = lookupStruct(bt->name, curScope);
            if (!si)
                err(node, "Struct type not found");
            else if (node.initList.size() > si->fields.size())
                err(node, "Too many initializers");
            else
                for (std::size_t i = 0; i < node.initList.size(); ++i) {
                    node.initList[i]->accept(*this);
                    Type* ft = std::get<varInfo>(si->fields[i].second.info).varType;
                    checkTypes(ft, curType, *node.initList[i], "Incompatible type in initializer");
                    if (curScope->parent)
                        coerce(node.initList[i], ft);
                }
        } else
            err(node, "Init list on scalar type");
    }

    curScope->symbols.emplace(node.name, di);
}

void AstAnalyser::visit(StructDecl& node) {
    if (curScope->symbols.find(node.name) != curScope->symbols.end()) {
        err(node, "Struct was declarated");
        return;
    }

    strctInfo si;

    for (auto& f : node.fields) {
        si.fields.emplace_back(f->name, DeclInfo{varInfo{f->type.get(), f->init != nullptr}});
    }
    curScope->symbols.emplace(node.name, DeclInfo(si));
}

void AstAnalyser::analyseFuncBody(FuncDecl& node) {
    auto funcScope = new Scope;
    funcScope->parent = curScope;
    curScope = funcScope;

    for (auto& p : node.params)
        curScope->symbols.emplace(p->name, DeclInfo{varInfo{p->type.get(), true}});

    auto savedRetType = retType;
    retType = node.returnType.get();

    node.body->accept(*this);

    curScope = funcScope->parent;
    retType = savedRetType;
    delete funcScope;
}

// TODO split 2 for
void AstAnalyser::visit(FuncDecl& node) {
    node.mangled = mangleFunc(node); // same signature -> same symbol

    auto ft = std::make_unique<FuncType>();
    ft->returnType = node.returnType->clone();
    ft->variadic = node.variadic;
    for (auto& p : node.params)
        ft->params.push_back(p->type.get()->clone());

    auto f = curScope->symbols.find(node.name);
    if (f != curScope->symbols.end()) {
        auto existing = std::get_if<funcInfo>(&f->second.info);
        if (!existing) {
            err(node, "Name already declared as a non-function");
            return;
        }
        // an overload with this exact signature already exists?
        funcOverload* ov = nullptr;
        for (auto& o : existing->overloads)
            if (o.mangled == node.mangled) {
                ov = &o;
                break;
            }
        if (ov) {
            if (!node.body || ov->isDefined) {
                err(node, "Func was declarated");
                return;
            }
            if (ov->isExtern) {
                err(node, "Extern function cannot be defined");
                return;
            }
            ov->isDefined = true;
            analyseFuncBody(node);
            return;
        }
        // new overload: different parameter signature
        existing->overloads.push_back(
            {ft.get(), node.body != nullptr, node.isExtern, node.mangled});
        funcTypes.push_back(std::move(ft));
        if (node.body)
            analyseFuncBody(node);
        return;
    }

    funcInfo fi;
    fi.overloads.push_back({ft.get(), node.body != nullptr, node.isExtern, node.mangled});
    funcTypes.push_back(std::move(ft));
    curScope->symbols.emplace(node.name, DeclInfo(fi));
    if (node.body)
        analyseFuncBody(node);
}

void AstAnalyser::visit(BlockStmt& node) {
    auto blockScope = new Scope;
    blockScope->parent = curScope;
    curScope = blockScope;

    for (auto& s : node.statements) {
        s->accept(*this);
    }

    curScope = curScope->parent;
    delete blockScope;
}

void AstAnalyser::visit(ExprStmt& node) {
    if (node.expr)
        node.expr->accept(*this); // null stmt
}

// TODO cond -> bool

void AstAnalyser::visit(IfStmt& node) {
    node.cond->accept(*this);
    node.thenPart->accept(*this);
    if (node.elsePart)
        node.elsePart->accept(*this);
}

void AstAnalyser::visit(WhileStmt& node) {
    auto wasInLoop = isInLoop;
    isInLoop = true;
    node.cond->accept(*this);
    node.body->accept(*this);
    isInLoop = wasInLoop;
}

void AstAnalyser::visit(ForStmt& node) {
    auto wasInLoop = isInLoop;
    isInLoop = true;
    auto forScope = new Scope;
    forScope->parent = curScope;
    curScope = forScope;

    if (node.initDecl != nullptr)
        node.initDecl->accept(*this);
    else if (node.initStmt != nullptr)
        node.initStmt->accept(*this);
    if (node.cond != nullptr)
        node.cond->accept(*this);
    if (node.incr != nullptr)
        node.incr->accept(*this);
    node.body->accept(*this);

    curScope = curScope->parent;
    delete forScope;
    isInLoop = wasInLoop;
}

void AstAnalyser::visit(ReturnStmt& node) {
    if (node.value)
        node.value->accept(*this);
    else
        curType = &voidType;
    checkTypes(retType, curType, node, "Return type mismatch");
    if (node.value)
        coerce(node.value, retType);
}

void AstAnalyser::visit(BreakStmt& node) {
    if (!isInLoop) {
        err(node, "Break statement not at loop");
    }
}

void AstAnalyser::visit(ContinueStmt& node) {
    if (!isInLoop) {
        err(node, "Continue statement not at loop");
    }
}

void AstAnalyser::visit(DeclStmt& node) { node.decl->accept(*this); }

// Change assign, from binary to other AST node
void AstAnalyser::visit(BinaryExpr& node) {
    if (node.op >= BinaryOp::Assign) {
        if (auto id = dynamic_cast<Identifier*>(node.left.get())) {
            if (auto v = lookupVar(curScope, id->name); v && v->isConst)
                err(node, "Assign to const var");
            else if (!v && isFuncName(curScope, id->name))
                err(node, "Function is not assignable");
            if (node.op == BinaryOp::Assign)
                markInited(curScope, id->name);
        }
    }

    node.left->accept(*this);
    auto lType = curType;
    node.right->accept(*this);
    auto rType = curType;

    // raw data: only =, ==, != are allowed
    if ((isByte(lType) || isByte(rType)) && node.op != BinaryOp::Assign &&
        node.op != BinaryOp::Equal && node.op != BinaryOp::NotEqual)
        err(node, "byte is not arithmetic");

    // floats have no %, bitwise or shift
    if (auto b = dynamic_cast<BuiltinType*>(lType); b && b->type == BuiltinTypes::Float) {
        bool bad = node.op == BinaryOp::Mod ||
                   (node.op >= BinaryOp::BitAnd && node.op <= BinaryOp::Shr) ||
                   node.op >= BinaryOp::ModAssign;
        if (bad)
            err(node, "Invalid operator for float");
    }

    if (node.op >= BinaryOp::Assign) {
        // no stores through const int*
        if (auto u = dynamic_cast<UnaryExpr*>(node.left.get()); u && u->op == UnaryOp::Deref) {
            if (auto p = dynamic_cast<PointerType*>(u->child->resultType.get()); p && p->constBase)
                err(node, "Assign through const pointer");
        }
        if (auto ix = dynamic_cast<IndexExpr*>(node.left.get())) {
            if (auto p = dynamic_cast<PointerType*>(ix->arr->resultType.get()); p && p->constBase)
                err(node, "Assign through const pointer");
        }
    }

    checkTypes(lType, rType, node, "Incompatible types in binary expression");

    Type* common = lType;
    if (node.op >= BinaryOp::Assign) {
        coerce(node.right, lType);
    } else if (isFloatTy(lType) != isFloatTy(rType)) {
        common = isFloatTy(lType) ? lType : rType;
        coerce(node.left, common);
        coerce(node.right, common);
    }

    switch (node.op) {
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
        curType = common;
        break;
    }
    node.resultType = curType->clone();
}

// TODO portfix + assign check
// TODO weakptr
void AstAnalyser::visit(UnaryExpr& node) {
    if (node.op == UnaryOp::AddressOf) {
        if (auto id = dynamic_cast<Identifier*>(node.child.get())) {
            markInited(curScope, id->name);
        }
    }

    node.child->accept(*this);

    if (isByte(curType) && node.op != UnaryOp::AddressOf)
        err(node, "byte is not arithmetic");

    if (node.op >= UnaryOp::PreInc && node.op <= UnaryOp::PostDec) {
        if (auto b = dynamic_cast<BuiltinType*>(curType); b && b->type == BuiltinTypes::Float)
            err(node, "++/-- is not supported for float");
        if (auto id = dynamic_cast<Identifier*>(node.child.get())) {
            if (auto v = lookupVar(curScope, id->name); v && v->isConst)
                err(node, "Assign to const var");
        } else if (!dynamic_cast<IndexExpr*>(node.child.get()) &&
                   !dynamic_cast<AccessExpr*>(node.child.get())) {
            auto u = dynamic_cast<UnaryExpr*>(node.child.get());
            if (!u || u->op != UnaryOp::Deref)
                err(node, "++/-- needs an lvalue");
        }
    }

    switch (node.op) {
    case UnaryOp::Pos:
    case UnaryOp::Neg:
    case UnaryOp::PreInc:
    case UnaryOp::PreDec:
    case UnaryOp::PostInc:
    case UnaryOp::PostDec:
        break;
    case UnaryOp::Not:
    case UnaryOp::BitNot:
        curType = &intType;
        break;
    case UnaryOp::AddressOf: {
        if (curType == nullptr) {
            err(node, "Cannot take address of unknown type");
            break;
        }
        auto pt = std::make_unique<PointerType>();
        pt->base = curType->clone();
        // &const-var gives const int*
        if (auto id = dynamic_cast<Identifier*>(node.child.get())) {
            if (auto v = lookupVar(curScope, id->name); v && v->isConst)
                pt->constBase = true;
        }
        curType = pt.get();
        ptrTypes.push_back(std::move(pt));
        break;
    }
    case UnaryOp::Deref:
        // TODO void pointer deref
        if (auto pt = dynamic_cast<PointerType*>(curType)) {
            curType = pt->base.get();
            if (auto tmp = dynamic_cast<BuiltinType*>(curType);
                tmp && tmp->type == BuiltinTypes::Void) {
                err(node, "Deref of void pointer");
            }
        } else {
            err(node, "Deref of not a pointer");
        }
        break;
    }
    if (curType)
        node.resultType = curType->clone();
}

void AstAnalyser::visit(CallExpr& node) {
    // direct call to a (possibly overloaded) named function — unless a local
    // variable of the same name shadows it (then it's an indirect call).
    auto* id = dynamic_cast<Identifier*>(node.func.get());
    funcInfo* fi = nullptr;
    if (id)
        for (Scope* s = curScope; s; s = s->parent) {
            auto it = s->symbols.find(id->name);
            if (it == s->symbols.end())
                continue;
            fi = std::get_if<funcInfo>(&it->second.info); // null if it's a variable
            break;                                        // nearest binding wins
        }

    if (fi) {
        std::vector<Type*> argTypes;
        argTypes.reserve(node.param.size());
        for (auto& a : node.param) {
            a->accept(*this);
            argTypes.push_back(curType);
        }

        // one candidate: use it (arguments may convert, like an ordinary call).
        // several: pick the exact match only (A.2.8 — no implicit conversions).
        funcOverload* chosen = nullptr;
        if (fi->overloads.size() == 1) {
            chosen = &fi->overloads[0];
        } else {
            int matches = 0;
            for (auto& o : fi->overloads) {
                FuncType* ft = o.funcType;
                bool ok = ft->variadic ? argTypes.size() >= ft->params.size()
                                       : argTypes.size() == ft->params.size();
                for (std::size_t i = 0; ok && i < ft->params.size(); ++i)
                    if (checkCast(argTypes[i], ft->params[i].get()) != castResult::Equal)
                        ok = false;
                if (ok) {
                    chosen = &o;
                    ++matches;
                }
            }
            if (matches != 1) {
                err(node,
                    matches == 0 ? "No matching function overload" : "Ambiguous overloaded call");
                curType = nullptr;
                return;
            }
        }

        FuncType* ft = chosen->funcType;
        bool countOk = ft->variadic ? node.param.size() >= ft->params.size()
                                    : node.param.size() == ft->params.size();
        if (!countOk) {
            err(node, "Declarated number of arguments not equal");
        } else {
            for (std::size_t i = 0; i < node.param.size(); ++i)
                if (i < ft->params.size()) {
                    checkTypes(ft->params[i].get(), argTypes[i], *node.param[i],
                               "Incompatible argument type");
                    coerce(node.param[i], ft->params[i].get());
                }
        }
        id->resolvedSym = chosen->mangled;
        curType = ft->returnType.get();
        node.resultType = curType->clone();
        return;
    }

    // indirect call (through a function-pointer value)
    node.func->accept(*this);
    auto fType = dynamic_cast<FuncType*>(curType);
    if (fType == nullptr) {
        err(node, "Called object is not a function");
        curType = nullptr;
        return;
    }
    bool countOk = fType->variadic ? node.param.size() >= fType->params.size()
                                   : node.param.size() == fType->params.size();
    if (!countOk) {
        err(node, "Declarated number of arguments not equal");
    } else {
        for (std::size_t i = 0; i < node.param.size(); ++i) {
            node.param[i]->accept(*this);
            if (i < fType->params.size()) {
                checkTypes(fType->params[i].get(), curType, *node.param[i],
                           "Incompatible argument type");
                coerce(node.param[i], fType->params[i].get());
            }
        }
        curType = fType->returnType.get();
    }
    node.resultType = curType ? curType->clone() : nullptr;
}

void AstAnalyser::visit(CastExpr& node) {
    node.expr->accept(*this);
    // byte: explicit cast, int/char only
    bool byteCast = (isByte(node.target.get()) || isByte(curType)) &&
                    typeIndex(node.target.get()) != -1 && typeIndex(curType) != -1 &&
                    typeIndex(node.target.get()) != 1 && typeIndex(curType) != 1; // not float
    if (!byteCast)
        checkTypes(node.target.get(), curType, node, "Incompatible cast types");
    curType = node.target.get();
    node.resultType = curType->clone();
}

void AstAnalyser::visit(IndexExpr& node) {
    node.index->accept(*this);
    checkTypes(curType, &intType, node, "Must be integer in Array");
    node.arr->accept(*this);
    if (auto at = dynamic_cast<ArrayType*>(curType)) {
        curType = at->elemType.get();
    } else if (auto pt = dynamic_cast<PointerType*>(curType))
        curType = pt->base.get();
    else {
        err(node, "Not Array");
    }
    node.resultType = curType->clone();
}

strctInfo* lookupStruct(std::string_view name, Scope* curScope) {
    for (Scope* s = curScope; s; s = s->parent) {
        auto it = s->symbols.find(name);
        if (it == s->symbols.end())
            continue;
        return std::get_if<strctInfo>(&it->second.info);
    }
    return nullptr;
}

void AstAnalyser::visit(AccessExpr& node) {
    node.object->accept(*this);

    BuiltinType* bt = nullptr;
    if (node.kind == AccessKind::Dot) {
        bt = dynamic_cast<BuiltinType*>(curType);
        if (!bt || bt->type != BuiltinTypes::Custom) {
            err(node, "Member access on non-struct type");
            return;
        }
    } else {
        auto pt = dynamic_cast<PointerType*>(curType);
        if (!pt) {
            err(node, "Arrow on non-pointer");
            return;
        }
        bt = dynamic_cast<BuiltinType*>(pt->base.get());
        if (!bt || bt->type != BuiltinTypes::Custom) {
            err(node, "Member access on non-struct type");
            return;
        }
    }

    auto si = lookupStruct(bt->name, curScope);
    if (!si) {
        err(node, "Struct type not found");
        return;
    }

    for (auto& f : si->fields) {
        if (f.first == node.field) {
            curType = std::get<varInfo>(f.second.info).varType;
            node.resultType = curType->clone();
            return;
        }
    }
    err(node, "Field not found in struct");
}

void AstAnalyser::visit(SizeofExpr& node) {
    if (node.expr)
        node.expr->accept(*this);
    curType = &intType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(TypeidExpr& node) {
    node.expr->accept(*this);
    auto pt = std::make_unique<PointerType>(charType.clone());
    curType = pt.get();
    node.resultType = pt->clone();
    ptrTypes.push_back(std::move(pt));
}

void AstAnalyser::visit(IntLiteral& node) {
    (void)node;
    curType = &intType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(FloatLiteral& node) {
    (void)node;
    curType = &floatType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(CharLiteral& node) {
    (void)node;
    curType = &charType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(BoolLiteral& node) {
    (void)node;
    curType = &boolType;
    node.resultType = curType->clone();
}

void AstAnalyser::visit(StringLiteral& node) {
    (void)node;
    auto pt = std::make_unique<PointerType>();
    pt->base = charType.clone();
    curType = pt.get();
    ptrTypes.push_back(std::move(pt));
    node.resultType = curType->clone();
}

// void*
void AstAnalyser::visit(NullLiteral& node) {
    auto pt = std::make_unique<PointerType>(voidType.clone());
    curType = pt.get();
    ptrTypes.push_back(std::move(pt));
    node.resultType = curType->clone();
}

void AstAnalyser::visit(Identifier& node) {
    for (Scope* s = curScope; s; s = s->parent) {
        auto it = s->symbols.find(node.name);
        if (it == s->symbols.end())
            continue;
        if (auto v = std::get_if<varInfo>(&it->second.info)) {
            curType = v->varType;
            node.resultType = curType->clone();
            if (!v->inited) {
                err(node, "Var is not inited");
            }
            return;
        }
        if (auto f = std::get_if<funcInfo>(&it->second.info)) {
            // a function name in value position (e.g. taking its address) must
            // pick a single overload; with several there's no context to choose.
            if (f->overloads.size() != 1) {
                err(node, "Ambiguous reference to overloaded function");
                return;
            }
            curType = f->overloads[0].funcType;
            node.resolvedSym = f->overloads[0].mangled;
            node.resultType = curType->clone();
            return;
        }

        continue;
    }
    err(node, "Identifier not found");
}

void AstAnalyser::visit(BuiltinType& node) { (void)node; }

void AstAnalyser::visit(PointerType& node) { (void)node; }

void AstAnalyser::visit(FuncType& node) { (void)node; }

void AstAnalyser::visit(ArrayType& node) { (void)node; }