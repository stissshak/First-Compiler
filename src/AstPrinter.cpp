//MPL/src/AstPrinter.cpp

#include <iostream>
#include <vector>

#include "AstPrinter.hpp"
#include "Ast.hpp"

void AstPrinter::printIndent(bool last) {
    for (int i = 0; i < (int)isLast.size() - 1; ++i) {
        std::cout << (isLast[i] ? "    " : "│   ");
    }
    if (!isLast.empty()) {
        std::cout << (last ? "└── " : "├── ");
    }
}

void AstPrinter::enter(bool last) {
    isLast.push_back(last);
}

void AstPrinter::leave() {
    isLast.pop_back();
}

std::string AstPrinter::typeName(Type* type) {
    if (!type) return "null";
    if (auto* b = dynamic_cast<BuiltinType*>(type)) {
        switch (b->type) {
            case BuiltinTypes::Int:   return "int";
            case BuiltinTypes::Float: return "float";
            case BuiltinTypes::Char:  return "char";
            case BuiltinTypes::Void:  return "void";
            case BuiltinTypes::Custom: return std::string{b->name};
        }
    }
    if (auto* p = dynamic_cast<PointerType*>(type)) {
        return typeName(p->base.get()) + "*";
    }
    return "?";
}

std::string AstPrinter::binaryOpName(BinaryOp op) {
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

std::string AstPrinter::unaryOpName(UnaryOp op) {
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

void AstPrinter::print(TranslationUnit& unit) {
    unit.accept(*this);
}

//----------------------------------------
// Decl

void AstPrinter::visit(TranslationUnit& node) {
    std::cout << "TranslationUnit\n";
    for (std::size_t i = 0; i < node.decls.size(); ++i) {
        bool last = (i == node.decls.size() - 1);
        printIndent(last);
        enter(last);
        node.decls[i]->accept(*this);
        leave();
    }
}

void AstPrinter::visit(StructDecl& node){
    std::cout << "StructDecl '" << node.name << "'\n";
    {
        bool last = false;
        printIndent(last);
        std::cout << "Fields";
        if (node.fields.empty()) {
            std::cout << ": (none)\n";
        } else {
            std::cout << "\n";
            enter(last);
            for (std::size_t i = 0; i < node.fields.size(); ++i) {
                bool l = (i == node.fields.size() - 1);
                printIndent(l);
                enter(l);
                node.fields[i]->accept(*this);
                leave();
            }
            leave();
        }
    } 
}

void AstPrinter::visit(FuncDecl& node) {
    std::cout << "FuncDecl '" << node.name
              << "' -> " << typeName(node.returnType.get()) << "\n";

    // params
    {
        bool last = false; // body всегда идёт после
        printIndent(last);
        std::cout << "Params";
        if (node.params.empty()) {
            std::cout << ": (none)\n";
        } else {
            std::cout << "\n";
            enter(last);
            for (std::size_t i = 0; i < node.params.size(); ++i) {
                bool l = (i == node.params.size() - 1);
                printIndent(l);
                enter(l);
                node.params[i]->accept(*this);
                leave();
            }
            leave();
        }
    }

    // body
    {
        printIndent(true);
        enter(true);
        node.body->accept(*this);
        leave();
    }
}

void AstPrinter::visit(VarDecl& node) {
    std::cout << "VarDecl '" << node.name
              << "' : " << typeName(node.type.get());
    if (node.init) {
        std::cout << "\n";
        printIndent(true);
        enter(true);
        node.init->accept(*this);
        leave();
    } else {
        std::cout << "\n";
    }
}

//----------------------------------------
// Stmt

void AstPrinter::visit(BlockStmt& node) {
    std::cout << "BlockStmt\n";
    for (std::size_t i = 0; i < node.statements.size(); ++i) {
        bool last = (i == node.statements.size() - 1);
        printIndent(last);
        enter(last);
        node.statements[i]->accept(*this);
        leave();
    }
}

void AstPrinter::visit(DeclStmt& node) {
    std::cout << "DeclStmt\n";
    printIndent(true);
    enter(true);
    node.decl->accept(*this);
    leave();
}

void AstPrinter::visit(ExprStmt& node) {
    std::cout << "ExprStmt\n";
    printIndent(true);
    enter(true);
    node.expr->accept(*this);
    leave();
}

void AstPrinter::visit(IfStmt& node) {
    std::cout << "IfStmt\n";

    bool hasElse = node.elsePart != nullptr;

    printIndent(false);
    std::cout << "Cond\n";
    enter(false);
    printIndent(true); enter(true);
    node.cond->accept(*this);
    leave(); leave();

    printIndent(!hasElse);
    std::cout << "Then\n";
    enter(!hasElse);
    printIndent(true); enter(true);
    node.thenPart->accept(*this);
    leave(); leave();

    if (hasElse) {
        printIndent(true);
        std::cout << "Else\n";
        enter(true);
        printIndent(true); enter(true);
        node.elsePart->accept(*this);
        leave(); leave();
    }
}

void AstPrinter::visit(WhileStmt& node) {
    std::cout << "WhileStmt\n";

    printIndent(false);
    std::cout << "Cond\n";
    enter(false);
    printIndent(true); enter(true);
    node.cond->accept(*this);
    leave(); leave();

    printIndent(true);
    std::cout << "Body\n";
    enter(true);
    printIndent(true); enter(true);
    node.body->accept(*this);
    leave(); leave();
}

void AstPrinter::visit(ForStmt& node) {
    std::cout << "ForStmt\n";

    // init
    printIndent(false);
    std::cout << "Init\n";
    enter(false);
    if (node.initDecl) {
        printIndent(true); enter(true);
        node.initDecl->accept(*this);
        leave();
    } else if(node.initStmt){
        printIndent(true); enter(true);
        node.initStmt->accept(*this);
        leave();
    }else{
        printIndent(true);
        std::cout << "(empty)\n";
    }
    leave();

    // cond
    printIndent(false);
    std::cout << "Cond\n";
    enter(false);
    if (node.cond) {
        printIndent(true); enter(true);
        node.cond->accept(*this);
        leave();
    } else {
        printIndent(true);
        std::cout << "(empty)\n";
    }
    leave();

    // incr
    printIndent(false);
    std::cout << "Incr\n";
    enter(false);
    if (node.incr) {
        printIndent(true); enter(true);
        node.incr->accept(*this);
        leave();
    } else {
        printIndent(true);
        std::cout << "(empty)\n";
    }
    leave();

    // body
    printIndent(true);
    std::cout << "Body\n";
    enter(true);
    printIndent(true); enter(true);
    node.body->accept(*this);
    leave(); leave();
}

void AstPrinter::visit(ReturnStmt& node) {
    std::cout << "ReturnStmt\n";
    if (node.value) {
        printIndent(true);
        enter(true);
        node.value->accept(*this);
        leave();
    }
}

void AstPrinter::visit(BreakStmt&)    { std::cout << "BreakStmt\n"; }
void AstPrinter::visit(ContinueStmt&) { std::cout << "ContinueStmt\n"; }

//----------------------------------------
// Expr

void AstPrinter::visit(BinaryExpr& node) {
    std::cout << "BinaryExpr '" << binaryOpName(node.op) << "'\n";

    printIndent(false);
    enter(false);
    node.left->accept(*this);
    leave();

    printIndent(true);
    enter(true);
    node.right->accept(*this);
    leave();
}

void AstPrinter::visit(UnaryExpr& node) {
    std::cout << "UnaryExpr '" << unaryOpName(node.op) << "'\n";
    printIndent(true);
    enter(true);
    node.child->accept(*this);
    leave();
}

void AstPrinter::visit(CallExpr& node) {
    std::cout << "CallExpr\n";

    bool hasArgs = !node.param.empty();

    printIndent(!hasArgs);
    std::cout << "Func\n";
    enter(!hasArgs);
    printIndent(true); enter(true);
    node.func->accept(*this);
    leave(); leave();

    if (hasArgs) {
        printIndent(true);
        std::cout << "Args\n";
        enter(true);
        for (std::size_t i = 0; i < node.param.size(); ++i) {
            bool last = (i == node.param.size() - 1);
            printIndent(last);
            enter(last);
            node.param[i]->accept(*this);
            leave();
        }
        leave();
    }
}

void AstPrinter::visit(CastExpr& node) {
    std::cout << "CaseExpr\n";

    printIndent(true);
    std::cout << "From\n";
    node.target->accept(*this);
    printIndent(true);
    std::cout << "To\n";
    node.expr->accept(*this);
    leave(); leave();
}

void AstPrinter::visit(IndexExpr& node) {
    std::cout << "IndexExpr\n";

    printIndent(false);
    std::cout << "Array\n";
    enter(false);
    printIndent(true); enter(true);
    node.arr->accept(*this);
    leave(); leave();

    printIndent(true);
    std::cout << "Index\n";
    enter(true);
    printIndent(true); enter(true);
    node.index->accept(*this);
    leave(); leave();
}

void AstPrinter::visit(AccessExpr& node) {
    std::cout << "AccessExpr "
              << (node.kind == AccessKind::Dot ? "." : "->")
              << " '" << node.field << "'\n";
    printIndent(true);
    enter(true);
    node.object->accept(*this);
    leave();
}

void AstPrinter::visit(IntLiteral& node) {
    std::cout << "IntLiteral " << node.value << "\n";
}

void AstPrinter::visit(FloatLiteral& node) {
    std::cout << "FloatLiteral " << node.value << "\n";
}

void AstPrinter::visit(CharLiteral& node) {
    std::cout << "CharLiteral " << node.value << "\n";
}

void AstPrinter::visit(StringLiteral& node) {
    std::cout << "StringLiteral \"" << node.value << "\"\n";
}

void AstPrinter::visit(Identifier& node) {
    std::cout << "Identifier '" << node.name << "'\n";
}



void AstPrinter::visit(BuiltinType&) { std::cout << "BuiltinType\n"; }
void AstPrinter::visit(PointerType&) { std::cout << "PointerType\n"; }
void AstPrinter::visit(ArrayType&) {std::cout << "ArrayType\n"; }
void AstPrinter::visit(FuncType&) {std::cout << "FuncType\n";}