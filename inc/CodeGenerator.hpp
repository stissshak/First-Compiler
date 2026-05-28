// MPL/inc/CodeGeneraotr.hpp

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>

#include "AstVisitor.hpp"
#include "Ast.hpp"

struct VarInfo;
struct StackVarInfo;
struct FuncInfo;

class CodeGenerator : public AstVisitor{
public:
    CodeGenerator(const std::string &path){
        auto canon = std::filesystem::weakly_canonical(path).string();
        output.open(canon);
    }

    void generate(TranslationUnit& unit);
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
    void visit(CastExpr&)        override;
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

    FuncType* fType = nullptr;

    std::unordered_map<std::string_view, FuncDecl*> funcs;

    std::ofstream output;
    std::ofstream textOut, dataOut, bssOut, rodataOut;
    // Hashmap: 
};


