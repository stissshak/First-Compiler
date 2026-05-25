// MPL/src/CodeGenerator.cpp

#include "CodeGenerator.hpp"

enum sections{
    bss,
    data,
    rodata,
    text
};



struct VarInfo{
    std::string_view name;
    sections location;
    unsigned char size;
};

struct StackVarInfo{
    std::size_t offset;
    unsigned char size;
};





std::string typeToString(Type *t){

}

std::string typeBssToString(Type *t){

}


void CodeGenerator::generate(TranslationUnit& unit){
    unit.accept(*this);
}

void CodeGenerator::visit(TranslationUnit& node){
    for(std::size_t i = 0; i < node.decls.size(); ++i){
        node.decls[i]->accept(*this);
    }
}

void CodeGenerator::visit(VarDecl& node){
    if(inFunc){
        
    }
    else{
        if(node.init != nullptr){
            dataOut << node.name << typeToString(node.type.get()) /*+ init*/ << "\n" ;
        }
        else{
            bssOut << node.name << typeBssToString(node.type.get()) << "\n";
        }
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
    output << "";

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

