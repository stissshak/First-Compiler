// MPL/inc/AstVisitor.hpp

#pragma once

struct TranslationUnit;
struct VarDecl; struct FuncDecl; struct StructDecl;
struct BlockStmt; struct ExprStmt; struct IfStmt;
struct WhileStmt; struct ForStmt; struct ReturnStmt;
struct BreakStmt; struct ContinueStmt; struct DeclStmt;
struct BinaryExpr; struct UnaryExpr; struct CallExpr;
struct IndexExpr; struct AccessExpr;
struct IntLiteral; struct FloatLiteral; struct StringLiteral;
struct Identifier;
struct BuiltinType; struct PointerType; struct ArrayType;

struct AstVisitor{
    virtual ~AstVisitor() = default;

    virtual void visit(TranslationUnit&) = 0;
    virtual void visit(VarDecl&) = 0;
    virtual void visit(StructDecl&) = 0;
    virtual void visit(FuncDecl&) = 0;

    virtual void visit(BlockStmt&) = 0;
    virtual void visit(ExprStmt&) = 0;
    virtual void visit(IfStmt&) = 0;
    virtual void visit(WhileStmt&) = 0;
    virtual void visit(ForStmt&) = 0;
    virtual void visit(ReturnStmt&) = 0;
    virtual void visit(BreakStmt&) = 0;
    virtual void visit(ContinueStmt&) = 0;
    virtual void visit(DeclStmt&) = 0;

    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(UnaryExpr&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(IndexExpr&) = 0;
    virtual void visit(AccessExpr&) = 0;
    virtual void visit(IntLiteral&) = 0;
    virtual void visit(FloatLiteral&) = 0;
    virtual void visit(StringLiteral&) = 0;
    virtual void visit(Identifier&) = 0;

    virtual void visit(BuiltinType&) = 0;
    virtual void visit(PointerType&) = 0;
};