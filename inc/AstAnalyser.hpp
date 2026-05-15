// MPL/inc/AstAnalyser.hpp

#pragma once

#include <string>
#include <vector>

#include "AstVisitor.hpp"
#include "Ast.hpp"

struct DeclInfo;
struct Scope;

class AstAnalyser : public AstVisitor{
public:
    void analyse(TranslationUnit& unit);

private:
    void checkTypes(Type* a, Type* b, std::string_view msg);

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

    Scope *curScope = nullptr;
    Type *curType = nullptr, *retType = nullptr;
    bool isInLoop = false;
    
    // func type
    // curnt type

    BuiltinType intType{BuiltinTypes::Int, ""};
    BuiltinType floatType{BuiltinTypes::Float, ""};
    BuiltinType charType{BuiltinTypes::Char, ""};
    BuiltinType voidType{BuiltinTypes::Void, ""};
    PointerType pointType;

    std::vector<std::unique_ptr<FuncType>> funcTypes;
    std::vector<std::unique_ptr<PointerType>> ptrTypes;
};


