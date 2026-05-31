// MPL/inc/CodeGeneraotr.hpp

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <cstdint>

#include "AstVisitor.hpp"
#include "Ast.hpp"

enum class Section : uint8_t{Bss, Data, Rodata, Text};
enum class Storage : uint8_t{Global, Stack, Register};

enum class Reg : uint8_t{
    rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp,
    r8, r9, r10, r11, r12, r13, r14, r15
};


struct VarInfo{
    std::string_view name;
    uint32_t size = 0;
    uint32_t align = 0;
    Storage  storage = Storage::Stack;
    union{
        Section section;
        int32_t rbpOffset;
        int reg;
    };
};

struct FuncInfo{
    std::vector<VarInfo> vars;
    std::string body;
    uint32_t frameSize = 0;
    bool hasCall = false;
    std::vector<Reg> freeRegs;
    std::vector<Reg> inUse;
};

class CodeGenerator : public AstVisitor{
public:
    CodeGenerator(const std::string &path){
        auto canon = std::filesystem::weakly_canonical(path).string();
        output.open(canon);
    }

    void generate(TranslationUnit& unit);
private:
    void emitFunction(FuncDecl&);

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

    FuncInfo current;

    std::unordered_map<std::string_view, FuncDecl*> funcs;

    std::ofstream output;
    std::string textBuf, dataBuf, bssBuf, rodataBuf;
    // Hashmap: 
};


