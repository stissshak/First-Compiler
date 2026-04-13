//MPL/src/AstIntter.cpp

#include <iostream>
#include <vector>

#include "AstInter.hpp"
#include "Ast.hpp"


void AstInter::enter(bool last) {
    isLast.push_back(last);
}

void AstInter::leave() {
    isLast.pop_back();
}

std::string AstInter::typeName(Type* type) {
    if (!type) return "null";
    if (auto* b = dynamic_cast<BuiltinType*>(type)) {
        switch (b->type) {
            case BuiltinTypes::Int:   return "int";
            case BuiltinTypes::Float: return "float";
            case BuiltinTypes::Char:  return "char";
            case BuiltinTypes::Void:  return "void";
        }
    }
    if (auto* p = dynamic_cast<PointerType*>(type)) {
        return typeName(p->base.get()) + "*";
    }
    return "?";
}

std::string AstInter::binaryOpName(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:          return "+";
        case BinaryOp::Sub:          return "-";
        case BinaryOp::Mul:          return "*";
        case BinaryOp::Div:          return "/";
        case BinaryOp::Mod:          return "%";
        case BinaryOp::Less:         return "<";
        case BinaryOp::Greater:      return ">";
        case BinaryOp::LessEqual:    return "<=";
        case BinaryOp::GreaterEqual: return ">=";
        case BinaryOp::Equal:        return "==";
        case BinaryOp::NotEqual:     return "!=";
        case BinaryOp::And:          return "&&";
        case BinaryOp::Or:           return "||";
        case BinaryOp::Assign:       return "=";
        case BinaryOp::AddAssign:    return "+=";
        case BinaryOp::MinusAssign:  return "-=";
    }
    return "?";
}

std::string AstInter::unaryOpName(UnaryOp op) {
    switch (op) {
        case UnaryOp::Pos:       return "+";
        case UnaryOp::Neg:       return "-";
        case UnaryOp::Not:       return "!";
        case UnaryOp::AddressOf: return "&";
        case UnaryOp::Deref:     return "*";
        case UnaryOp::PreInc:    return "++ (pre)";
        case UnaryOp::PreDec:    return "-- (pre)";
        case UnaryOp::PostInc:   return "++ (post)";
        case UnaryOp::PostDec:   return "-- (post)";
    }
    return "?";
}

void AstInter::inter(TranslationUnit& unit) {
    unit.accept(*this);
}

//----------------------------------------
// Decl

void AstInter::visit(TranslationUnit& node) {
    std::cout << "TranslationUnit\n";
    for (std::size_t i = 0; i < node.decls.size(); ++i) {
        bool last = (i == node.decls.size() - 1);
        
        enter(last);
        node.decls[i]->accept(*this);
        leave();
    }
}

void AstInter::visit(FuncDecl& node) {
    std::cout << "FuncDecl '" << node.name
              << "' -> " << typeName(node.returnType.get()) << "\n";

    // params
    {
        bool last = false; // body всегда идёт после
        
        std::cout << "Params";
        if (node.params.empty()) {
            std::cout << ": (none)\n";
        } else {
            std::cout << "\n";
            enter(last);
            for (std::size_t i = 0; i < node.params.size(); ++i) {
                bool l = (i == node.params.size() - 1);
                
                enter(l);
                node.params[i]->accept(*this);
                leave();
            }
            leave();
        }
    }

    // body
    {
        
        enter(true);
        node.body->accept(*this);
        leave();
    }
}

void AstInter::visit(VarDecl& node) {
    std::cout << "VarDecl '" << node.name
              << "' : " << typeName(node.type.get());
    if (node.init) {
        std::cout << "\n";
        
        enter(true);
        node.init->accept(*this);
        leave();
    } else {
        std::cout << "\n";
    }
}

//----------------------------------------
// Stmt

void AstInter::visit(BlockStmt& node) {
    std::cout << "BlockStmt\n";
    for (std::size_t i = 0; i < node.statements.size(); ++i) {
        bool last = (i == node.statements.size() - 1);
        
        enter(last);
        node.statements[i]->accept(*this);
        leave();
    }
}

void AstInter::visit(DeclStmt& node) {
    std::cout << "DeclStmt\n";
    
    enter(true);
    node.decl->accept(*this);
    leave();
}

void AstInter::visit(ExprStmt& node) {
    std::cout << "ExprStmt\n";
    
    enter(true);
    node.expr->accept(*this);
    leave();
}

void AstInter::visit(IfStmt& node) {
    std::cout << "IfStmt\n";

    bool hasElse = node.elsePart != nullptr;

    
    std::cout << "Cond\n";
    enter(false);
     enter(true);
    node.cond->accept(*this);
    leave(); leave();

    std::cout << "Then\n";
    enter(!hasElse);
     enter(true);
    node.thenPart->accept(*this);
    leave(); leave();

    if (hasElse) {
        
        std::cout << "Else\n";
        enter(true);
         enter(true);
        node.elsePart->accept(*this);
        leave(); leave();
    }
}

void AstInter::visit(WhileStmt& node) {
    std::cout << "WhileStmt\n";

    
    std::cout << "Cond\n";
    enter(false);
     enter(true);
    node.cond->accept(*this);
    leave(); leave();

    
    std::cout << "Body\n";
    enter(true);
     enter(true);
    node.body->accept(*this);
    leave(); leave();
}

void AstInter::visit(ForStmt& node) {
    std::cout << "ForStmt\n";

    // init
    
    std::cout << "Init\n";
    enter(false);
    if (node.init) {
         enter(true);
        node.init->accept(*this);
        leave();
    } else {
        
        std::cout << "(empty)\n";
    }
    leave();

    // cond
    
    std::cout << "Cond\n";
    enter(false);
    if (node.cond) {
         enter(true);
        node.cond->accept(*this);
        leave();
    } else {
        
        std::cout << "(empty)\n";
    }
    leave();

    // incr
    
    std::cout << "Incr\n";
    enter(false);
    if (node.incr) {
         enter(true);
        node.incr->accept(*this);
        leave();
    } else {
        
        std::cout << "(empty)\n";
    }
    leave();

    // body
    
    std::cout << "Body\n";
    enter(true);
     enter(true);
    node.body->accept(*this);
    leave(); leave();
}

void AstInter::visit(ReturnStmt& node) {
    std::cout << "ReturnStmt\n";
    if (node.value) {
        
        enter(true);
        node.value->accept(*this);
        leave();
    }
}

void AstInter::visit(BreakStmt&)    { std::cout << "BreakStmt\n"; }
void AstInter::visit(ContinueStmt&) { std::cout << "ContinueStmt\n"; }

//----------------------------------------
// Expr

void AstInter::visit(BinaryExpr& node) {
    std::cout << "BinaryExpr '" << binaryOpName(node.op) << "'\n";

    
    enter(false);
    node.left->accept(*this);
    leave();

    
    enter(true);
    node.right->accept(*this);
    leave();
}

void AstInter::visit(UnaryExpr& node) {
    std::cout << "UnaryExpr '" << unaryOpName(node.op) << "'\n";
    
    enter(true);
    node.child->accept(*this);
    leave();
}

void AstInter::visit(CallExpr& node) {
    std::cout << "CallExpr\n";

    bool hasArgs = !node.param.empty();

    std::cout << "Func\n";
    enter(!hasArgs);
     enter(true);
    node.func->accept(*this);
    leave(); leave();

    if (hasArgs) {
        
        std::cout << "Args\n";
        enter(true);
        for (std::size_t i = 0; i < node.param.size(); ++i) {
            bool last = (i == node.param.size() - 1);
            
            enter(last);
            node.param[i]->accept(*this);
            leave();
        }
        leave();
    }
}

void AstInter::visit(IndexExpr& node) {
    std::cout << "IndexExpr\n";

    
    std::cout << "Array\n";
    enter(false);
     enter(true);
    node.arr->accept(*this);
    leave(); leave();

    
    std::cout << "Index\n";
    enter(true);
     enter(true);
    node.index->accept(*this);
    leave(); leave();
}

void AstInter::visit(AccessExpr& node) {
    std::cout << "AccessExpr "
              << (node.kind == AccessKind::Dot ? "." : "->")
              << " '" << node.field << "'\n";
    
    enter(true);
    node.object->accept(*this);
    leave();
}

void AstInter::visit(IntLiteral& node) {
    std::cout << "IntLiteral " << node.value << "\n";
}

void AstInter::visit(FloatLiteral& node) {
    std::cout << "FloatLiteral " << node.value << "\n";
}

void AstInter::visit(StringLiteral& node) {
    std::cout << "StringLiteral \"" << node.value << "\"\n";
}

void AstInter::visit(Identifier& node) {
    std::cout << "Identifier '" << node.name << "'\n";
}



void AstInter::visit(BuiltinType&) { std::cout << "BuiltinType\n"; }
void AstInter::visit(PointerType&) { std::cout << "PointerType\n"; }