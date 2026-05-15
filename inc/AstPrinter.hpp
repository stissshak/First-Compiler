// MPL/inc/AstPrinter.hpp

#pragma once

#include <string>
#include <vector>

#include "AstVisitor.hpp"
#include "Ast.hpp"

class AstPrinter : public AstVisitor {
public:
    void print(TranslationUnit& unit);

private:
    std::vector<bool> isLast;

    void printIndent(bool last);
    void enter(bool last);
    void leave();

    std::string typeName(Type* type);
    std::string binaryOpName(BinaryOp op);
    std::string unaryOpName(UnaryOp op);

    void visit(TranslationUnit&) override;
    void visit(VarDecl&)         override;
    void visit(StructDecl&)      override;
    void visit(FuncDecl&)        override;
    void visit(BlockStmt&)       override;
    void visit(ExprStmt&)        override;
    void visit(IfStmt&)          override;
    void visit(WhileStmt&)       override;
    void visit(ForStmt&)         override;
    void visit(ReturnStmt&)      override;
    void visit(BreakStmt&)       override;
    void visit(ContinueStmt&)    override;
    void visit(DeclStmt&)        override;
    void visit(BinaryExpr&)      override;
    void visit(UnaryExpr&)       override;
    void visit(CallExpr&)        override;
    void visit(IndexExpr&)       override;
    void visit(AccessExpr&)      override;
    void visit(IntLiteral&)      override;
    void visit(FloatLiteral&)    override;
    void visit(CharLiteral&)     override;
    void visit(StringLiteral&)   override;
    void visit(Identifier&)      override;
    void visit(BuiltinType&)     override;
    void visit(PointerType&)     override;
    void visit(ArrayType&)       override;
    void visit(FuncType&)        override;

    int depth = 0;
    bool lastChild = true;
};