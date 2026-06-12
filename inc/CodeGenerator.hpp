// MPL/inc/CodeGeneraotr.hpp

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <algorithm>

#include "AstVisitor.hpp"
#include "Ast.hpp"
#include "SourceMap.hpp"

enum class Section : uint8_t{Bss, Data, Rodata, Text};
enum class Storage : uint8_t{Global, Stack, Register};

enum class Reg : uint8_t{
    rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp,
    r8, r9, r10, r11, r12, r13, r14, r15,
    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
    xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15,

};

enum class RegClass : uint8_t{ Int, Sse };

inline bool isXmm(Reg r){ return (uint8_t)r >= (uint8_t)Reg::xmm0; }

struct LValue{
    std::string mem;
    Reg reg;
    bool ownsReg = false;
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
    std::string name;
    std::string body;
    uint32_t frameSize = 0;
    std::size_t pushDepth = 0;   // qwords pushed past the frame, for call alignment
    bool hasCall = false;
    std::vector<Reg> freeRegs;
    std::vector<Reg> inUse;
    std::vector<Reg> freeXmm;
    std::vector<Reg> inUseXmm;
};

class CodeGenerator : public AstVisitor{
public:
    CodeGenerator(const std::string &path, const SourceMap& smap, const std::string& buffer)
        : smap(smap), buffer(buffer){
        auto canon = std::filesystem::weakly_canonical(path).string();
        output.open(canon);
    }

    void generate(TranslationUnit& unit);
private:
    Reg resultReg = Reg::rax;
    Reg alloc(RegClass cls = RegClass::Int);
    void freeReg(Reg r);
    VarInfo* findVar(std::string_view name);
    Reg emitRValue(Expr& e);
    LValue emitLValue(Expr& e);
    void emitLoad(Reg dst, const std::string& mem, Type* t);
    void emitStore(const std::string& mem, Reg src, Type* t);
    Reg loadLValue(const LValue& lv, Type* t);
    Reg lvalueAddr(Expr& e);
    void emitMemCopy(Reg dst, Reg src, uint32_t sz);
    void emitRtCheck(const std::string& jccOk, std::string_view msg, std::size_t off);
    static std::string memOf(int32_t off){ return "[rbp" + std::to_string(off) + "]"; }
    std::string newLabel(const std::string& tag){ return ".L" + tag + std::to_string(labelId++); }


    void emitFunction(FuncDecl&);
    void emitLocalVar(VarDecl&);
    Reg emitBaseAddr(Expr&);

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
    void visit(SizeofExpr&)      override;
    void visit(TypeidExpr&)      override;
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

    FuncInfo current;

    std::unordered_map<std::string_view, FuncDecl*> funcs;
    std::unordered_set<std::string> usedExterns;   // builtins actually called

    std::ofstream output;
    std::string textBuf, dataBuf, bssBuf, rodataBuf;
    
    std::size_t labelId = 0;
    std::vector<std::pair<std::string,std::string>> loopStack;

    const SourceMap& smap;
    const std::string& buffer;
    bool needRt = false;


};


