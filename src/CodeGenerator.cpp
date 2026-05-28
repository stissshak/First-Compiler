// MPL/src/CodeGenerator.cpp

#include "CodeGenerator.hpp"

enum class Section : uint8_t {Bss, Data, Rodata, Text};
enum class Storage : uint8_t {Global, Stack, Register};


struct VarInfo{
    std::string_view name;
    uint32_t size = 0;
    uint32_t align = 0;
    Storage  storage = Storage::Stack;
    union{
        Section section;
        uint32_t rbp;
        int reg;
    };
};

struct LocalVarInfo{
    std::size_t offset;
    uint32_t size, align;
};

// Func -> array var

struct FuncInfo{
      std::vector<VarInfo> vars;
      std::string instructions;
      uint32_t frameSize = 0;
  };



void CodeGenerator::generate(TranslationUnit& unit){
    unit.accept(*this);
}

void CodeGenerator::visit(TranslationUnit& node){
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
}

void CodeGenerator::visit(VarDecl& node){
    if(fType){

    }
}

void CodeGenerator::visit(StructDecl& node){

}

void CodeGenerator::visit(FuncDecl& node){
} 

void CodeGenerator::visit(BlockStmt& node){

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

