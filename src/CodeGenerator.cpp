// MPL/src/CodeGenerator.cpp

#include <string>
#include <vector>

#include "CodeGenerator.hpp"

inline uint32_t sizeOf(Type* t){
    if(auto b = dynamic_cast<BuiltinType*>(t)){
        switch(b->type){
            case BuiltinTypes::Int:   return 8;
            case BuiltinTypes::Float: return 8;
            case BuiltinTypes::Char:  return 1;
            case BuiltinTypes::Void:  return 0;
            case BuiltinTypes::Custom: return 0; // TODO: struct size
        }
    }
    if(dynamic_cast<PointerType*>(t)) return 8;
    if(auto a = dynamic_cast<ArrayType*>(t)) return a->size * sizeOf(a->elemType.get());
    return 0;
}

  inline uint32_t alignOf(Type* t){
    if(auto a = dynamic_cast<ArrayType*>(t)) return alignOf(a->elemType.get());
    return sizeOf(t) ? sizeOf(t) : 1; 
  }

void CodeGenerator::generate(TranslationUnit& unit){
    unit.accept(*this);

    if(!dataBuf.empty()) output << "section .data\n" << dataBuf << '\n';
    if(!rodataBuf.empty()) output << "section .rodata\n" << rodataBuf << '\n';
    if(!bssBuf.empty()) output << "section .bss\n" << bssBuf << '\n';
    output << "section .text\n" << textBuf;
}

void CodeGenerator::visit(TranslationUnit& node){
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
}

void CodeGenerator::visit(VarDecl& node){

}

void CodeGenerator::visit(StructDecl& node){

}

static void initFreeRegs(FuncInfo& f){
    f.freeRegs = {
        Reg::rbx, Reg::r12, Reg::r13, Reg::r14, Reg::r15,
        Reg::rcx, Reg::rsi, Reg::rdi, Reg::r8, Reg::r9, Reg::r10, Reg::r11,
    };
}

static const Reg argRegs[6] = {Reg::rdi, Reg::rsi, Reg::rdx, Reg::rcx, Reg::r8, Reg::r9};

static const char* regName(Reg r){
      static const char* n[16] = {
          "rax","rbx","rcx","rdx","rsi","rdi","rsp","rbp",
          "r8","r9","r10","r11","r12","r13","r14","r15",
      };
      return n[(uint8_t)r];
  }


static void assignParamOffsets(FuncDecl& node, FuncInfo& f){
    int32_t stackArg = 16;
    for(std::size_t i = 0; i < node.params.size(); ++i){
        VarDecl* p = node.params[i].get();
        uint32_t s = sizeOf(p->type.get());
        uint32_t a = alignOf(p->type.get());
        VarInfo vi{ p->name, s, a, Storage::Stack };
        if(i < 6){
            f.frameSize = (f.frameSize + 8 + 7) & ~7u;
            vi.rbpOffset = -(int32_t)f.frameSize;
            f.body += "\tmov [rbp" + std::to_string(vi.rbpOffset) + "], " + regName(argRegs[i]) + "\n";
        } else {
            vi.rbpOffset = stackArg;
            stackArg += 8;
        }
        f.vars.push_back(vi);
    }
}



void CodeGenerator::emitFunction(FuncDecl& node){
    uint32_t alignedN = (current.frameSize + 15) & ~15u;
    textBuf += "global ";  textBuf += node.name;  textBuf += '\n';

    textBuf += node.name;  textBuf += ":\n";

    textBuf += "\tpush rbp\n";
    textBuf += "\tmov rbp, rsp\n";
    if(alignedN > 0) textBuf += "\tsub rsp, " + std::to_string(alignedN) + '\n';

    textBuf += current.body;

    textBuf += ".Lreturn_"; textBuf += node.name;  textBuf += ":\n";
    textBuf += "\tleave\n";
    textBuf += "\tret\n\n";
}

void CodeGenerator::visit(FuncDecl& node){
    if(!node.body) return; 

    current = FuncInfo{};
    initFreeRegs(current);
    assignParamOffsets(node, current);

    node.body->accept(*this);

    emitFunction(node);
}


void CodeGenerator::visit(BlockStmt& node){
    std::size_t mark = current.vars.size();
    for(auto& stmt : node.statements){
        stmt->accept(*this);
    }
    current.vars.resize(mark);
}

void CodeGenerator::visit(ExprStmt& node){

}

void CodeGenerator::visit(IfStmt& node){

}

void CodeGenerator::visit(WhileStmt& node){

}

void CodeGenerator::visit(ForStmt& node){

}

void CodeGenerator::visit(ReturnStmt& node){

}

void CodeGenerator::visit(BreakStmt& node){

}

void CodeGenerator::visit(ContinueStmt& node){

}

void CodeGenerator::visit(DeclStmt& node){
    auto* vd = dynamic_cast<VarDecl*>(node.decl.get());
    if(!vd){ node.decl->accept(*this); return; }

    uint32_t s = sizeOf(vd->type.get());
    uint32_t a = alignOf(vd->type.get());

    current.frameSize = (current.frameSize + s + (a - 1)) & ~(a - 1);
    int32_t off = -(int32_t)current.frameSize;

    VarInfo vi{ vd->name, s, a, Storage::Stack };
    vi.rbpOffset = off;
    current.vars.push_back(vi);

}

void CodeGenerator::visit(BinaryExpr& node){

}

void CodeGenerator::visit(UnaryExpr& node){

}

void CodeGenerator::visit(CallExpr& node){

}

void CodeGenerator::visit(CastExpr& node){

}

void CodeGenerator::visit(IndexExpr& node){

}

void CodeGenerator::visit(AccessExpr& node){

}

void CodeGenerator::visit(IntLiteral& node){

}

void CodeGenerator::visit(FloatLiteral& node){

}

void CodeGenerator::visit(CharLiteral& node){

}

void CodeGenerator::visit(StringLiteral& node){

}

void CodeGenerator::visit(Identifier& node){

}

void CodeGenerator::visit(BuiltinType& node){

}

void CodeGenerator::visit(PointerType& node){

}

void CodeGenerator::visit(FuncType& node){

}

void CodeGenerator::visit(ArrayType& node){

}

