// MPL/src/AstAnalyser.cpp

#include <unordered_map>
#include <stack>
#include <optional>

#include "AstAnalyser.hpp"

// Таблица переменных/функций
// scope функции

enum class DeclType{
    Var, Func, Struct
};

struct varInfo{
    Type* varType;
    bool inited;
};


struct funcInfo{
    Type* retType;
    std::vector<Type*> argsType;
};

struct strctInfo{
    std::vector<std::pair<std::string_view, DeclInfo>> fields;
};

struct DeclInfo{
    std::variant<varInfo, funcInfo, strctInfo> info;
};

struct Scope{
    Scope *parent;
    std::unordered_map<std::string_view, DeclInfo> names;
};

void AstAnalyser::analyse(TranslationUnit& unit){
    unit.accept(*this);
}


void AstAnalyser::visit(TranslationUnit& node){
    prev = nullptr;
    main = new Scope;
    main->parent = nullptr;
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
}
void AstAnalyser::visit(VarDecl& node){
    DeclInfo di = {varInfo{node.type.get(), node.init != nullptr}};

    if (main->names.find(node.name) != main->names.end()) {
        // TODO Logger
        return;
    }
    main->names.emplace(node.name, di);
}

void AstAnalyser::visit(StructDecl& node){
    strctInfo si;

    for(auto& f : node.fields){
       si.fields.emplace_back(f->name, DeclInfo{varInfo{f->type.get(), f->init != nullptr }});
    }


    if (main->names.find(node.name) != main->names.end()) {
        // TODO Logger
        return;
    }
    main->names.emplace(node.name, DeclInfo(si));
}


void AstAnalyser::visit(FuncDecl& node){
    funcInfo fi;
    curType = node.returnType.get();

    for(auto& p: node.params){
        fi.argsType.emplace_back(p->type.get());
    }

    if (main->names.find(node.name) != main->names.end()) {
        // TODO Logger
        return;
    }
    main->names.emplace(node.name, DeclInfo(fi));

    auto funcScope = new Scope;
    funcScope->parent = main;
    main = funcScope;

    for (auto& p : node.params)
        main->names.emplace(p->name, DeclInfo{varInfo{p->type.get(), true}});

    node.body->accept(*this);
    delete funcScope;
}


void AstAnalyser::visit(BlockStmt& node){
    auto blockScope = new Scope;
    blockScope->parent = main;
    main = blockScope;

    for(auto& s: node.statements){
        s->accept(*this);
    }

    main = main->parent;
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
    isInLoop = true;
    node.cond->accept(*this);
    node.body->accept(*this);
    isInLoop = false;
}


void AstAnalyser::visit(ForStmt& node){
    isInLoop = true;
    auto forScope = new Scope;
    forScope->parent = main;
    main = forScope;

    if(node.init != nullptr) node.init->accept(*this);
    if(node.cond != nullptr) node.cond->accept(*this);
    if(node.incr != nullptr) node.incr->accept(*this);
    node.body->accept(*this);

    main = main->parent;
    delete forScope;
    isInLoop = false;
}


void AstAnalyser::visit(ReturnStmt& node){
    if(retType == nullptr){
        // Logger
    }
    if(node.value) node.value->accept(*this);
    else curType = &voidType;
    if(retType != curType){
        // TODO Logger
    }
}


void AstAnalyser::visit(BreakStmt& node){
    if(!isInLoop){
        // TODO Logger
    }
}


void AstAnalyser::visit(ContinueStmt& node){
    if(!isInLoop){
        // TODO Logger
    }
}


void AstAnalyser::visit(DeclStmt& node){
    node.decl->accept(*this);
}


void AstAnalyser::visit(BinaryExpr& node){
    node.left->accept(*this);
    auto lType = curType;
    node.right->accept(*this);
    auto rType = curType;

    if(lType != rType){
        // TODO Logger
    }
}


void AstAnalyser::visit(UnaryExpr& node){
    
}


void AstAnalyser::visit(CallExpr& node){
    node.func->accept(*this);
    auto fType = dynamic_cast<FuncType*>(curType);
    if(fType == nullptr){
        // TODO Logger
    }
    else{
        for(auto &p: node.param){

        }
        curType = fType->returnType.get();
    }
}


void AstAnalyser::visit(IndexExpr& node){

}


void AstAnalyser::visit(AccessExpr& node){
    node.
}


void AstAnalyser::visit(IntLiteral& node){
    curType = &intType;
}


void AstAnalyser::visit(FloatLiteral& node){
    curType = &floatType;
}

void AstAnalyser::visit(CharLiteral& node){
    curType = &charType;
}

void AstAnalyser::visit(StringLiteral& node){
    curType = &pointType;
    pointType.base = std::make_unique<BuiltinType>(BuiltinTypes::Char);
}


void AstAnalyser::visit(Identifier& node){
    for(Scope* s = main; s; s = s->parent){
          auto it = s->names.find(node.name);
          if (it == s->names.end()) continue;
          if (auto* v = std::get_if<varInfo>(&it->second.info)) {
              curType = v->varType;
              if (!v->inited) /* TODO Logger*/;
              return;
          }
          // TODO Logger
          return;
      }
      // TODO Logger
}


void AstAnalyser::visit(BuiltinType& node){

}


void AstAnalyser::visit(PointerType& node){

}