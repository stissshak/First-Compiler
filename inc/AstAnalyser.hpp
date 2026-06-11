// MPL/inc/AstAnalyser.hpp

#pragma once

#include <string>
#include <vector>

#include "AstVisitor.hpp"
#include "Ast.hpp"
#include "SourceMap.hpp"

struct DeclInfo;
struct Scope;

class AstAnalyser : public AstVisitor{
public:
    AstAnalyser(const SourceMap& smap, const std::string& buffer)
        : smap(smap), buffer(buffer) {}
    bool analyse(TranslationUnit& unit);   // false if errors

private:
    void err(std::string_view msg);
    void err(const Node& n, std::string_view msg);
    void warn(const Node& n, std::string_view msg);
    void checkTypes(Type* a, Type* b, const Node& n, std::string_view msg);
    void analyseFuncBody(FuncDecl& node);

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
    void visit(CastExpr&)        override;
    void visit(IndexExpr&)       override;
    void visit(AccessExpr&)      override;
    void visit(IntLiteral&)      override;
    void visit(FloatLiteral&)    override;
    void visit(CharLiteral&)     override;
    void visit(BoolLiteral&)     override;
    void visit(StringLiteral&)   override;
    void visit(Identifier&)      override;
    void visit(BuiltinType&)     override;
    void visit(PointerType&)     override;
    void visit(ArrayType&)       override;
    void visit(FuncType&)        override;

    Scope *curScope = nullptr;
    Type *curType = nullptr, *retType = nullptr;
    bool isInLoop = false;
    int errCount = 0;
    const SourceMap& smap;
    const std::string& buffer;
    
    // func type
    // curnt type

    BuiltinType intType{BuiltinTypes::Int, ""};
    BuiltinType floatType{BuiltinTypes::Float, ""};
    BuiltinType charType{BuiltinTypes::Char, ""};
    BuiltinType boolType{BuiltinTypes::Bool, ""};
    BuiltinType voidType{BuiltinTypes::Void, ""};
    PointerType pointType;

    std::vector<std::unique_ptr<FuncType>> funcTypes;
    std::vector<std::unique_ptr<PointerType>> ptrTypes;
};


