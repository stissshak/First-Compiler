// MPL/inc/AstAnalyser.hpp

#pragma once

#include <string>
#include <vector>

#include "AstVisitor.hpp"
#include "Ast.hpp"

struct DeclInfo;
struct Scope;

class AstAnalyser : public AstVisitor {
public:
    void analyse(TranslationUnit& unit);

private:
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

    Scope *main, *prev;
    Type *curType, *retType;
    bool isInLoop;
    
    // func type
    // curnt type

    BuiltinType intType{BuiltinTypes::Int, ""};
    BuiltinType floatType{BuiltinTypes::Float, ""};
    BuiltinType charType{BuiltinTypes::Char, ""};
    BuiltinType voidType{BuiltinTypes::Void, ""};
    BuiltinType customType{BuiltinTypes::Custom, ""};
    PointerType pointType{nullptr};
};



fmas[i](x)

fmas -> scope

fmas[i] - Func -> FuncType(x)