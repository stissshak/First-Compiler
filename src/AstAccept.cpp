#include "Ast.hpp"
#include "AstVisitor.hpp"

void TranslationUnit::accept(AstVisitor& v) { v.visit(*this); }
void VarDecl::accept(AstVisitor& v)         { v.visit(*this); }
void StructDecl::accept(AstVisitor& v)      { v.visit(*this); }
void FuncDecl::accept(AstVisitor& v)        { v.visit(*this); }
void BlockStmt::accept(AstVisitor& v)       { v.visit(*this); }
void ExprStmt::accept(AstVisitor& v)        { v.visit(*this); }
void DeclStmt::accept(AstVisitor& v)        { v.visit(*this); }
void IfStmt::accept(AstVisitor& v)          { v.visit(*this); }
void WhileStmt::accept(AstVisitor& v)       { v.visit(*this); }
void ForStmt::accept(AstVisitor& v)         { v.visit(*this); }
void ReturnStmt::accept(AstVisitor& v)      { v.visit(*this); }
void BreakStmt::accept(AstVisitor& v)       { v.visit(*this); }
void ContinueStmt::accept(AstVisitor& v)    { v.visit(*this); }
void BinaryExpr::accept(AstVisitor& v)      { v.visit(*this); }
void UnaryExpr::accept(AstVisitor& v)       { v.visit(*this); }
void CallExpr::accept(AstVisitor& v)        { v.visit(*this); }
void IndexExpr::accept(AstVisitor& v)       { v.visit(*this); }
void AccessExpr::accept(AstVisitor& v)      { v.visit(*this); }
void IntLiteral::accept(AstVisitor& v)      { v.visit(*this); }
void FloatLiteral::accept(AstVisitor& v)    { v.visit(*this); }
void CharLiteral::accept(AstVisitor& v)     { v.visit(*this); }
void StringLiteral::accept(AstVisitor& v)   { v.visit(*this); }
void Identifier::accept(AstVisitor& v)      { v.visit(*this); }
void BuiltinType::accept(AstVisitor& v)     { v.visit(*this); }
void PointerType::accept(AstVisitor& v)     { v.visit(*this); }
void ArrayType::accept(AstVisitor& v)     { v.visit(*this); }
void FuncType::accept(AstVisitor& v)     { v.visit(*this); }