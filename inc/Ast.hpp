// MPL/inc/Ast.hpp

#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <vector>

struct Node{
	virtual ~Node() = default;
};

struct Decl : Node {};
struct Stmt : Node {};
struct Expr : Node {};
struct Type : Node {};

// target
struct TranslationUnit : Node{
	std::vector<std::unique_ptr<Decl>> decls;
};

// Decl
struct VarDecl : Decl{
	std::unique_ptr<Type> type;
	std::string_view name;
	std::unique_ptr<Expr> init;
};

struct FuncDecl : Decl{
	std::unique_ptr<Type> returnType;
	std::string name;
	std::vector<std::unique_ptr<VarDecl>> params;
	std::unique_ptr<Stmt> body;
};

// Stmt
struct BlockStmt : Stmt{
	std::vector<std::unique_ptr<Stmt>> statements;
};

struct ExprStmt : Stmt {
	std::unique_ptr<Expr> expr;
};

struct IfStmt : Stmt{
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Stmt> thenPart;
	std::unique_ptr<Stmt> elsePart;
};

struct WhileStmt : Stmt{
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Stmt> body;
};

struct ForStmt : Stmt{
	std::unique_ptr<Node> init;
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Expr> incr;
	std::unique_ptr<Stmt> body;
};

struct ReturnStmt : Stmt{
	std::unique_ptr<Expr> value;
};

struct BreakStmt : Stmt{};
struct ContinueStmt : Stmt{};


// Expr
// Operations

enum class BinaryOp{
	Add, Sub, Mul, Div, Mod,
	Less, Greater, Equal, NotEqual,
	And, Or,
	Assign,	AddAssign, MinusAssign 

};

struct BinaryExpr : Expr{
	BinaryOp op;
	std::unique_ptr<Expr> left;
	std::unique_ptr<Expr> right;
};

enum class UnaryOp{
	Pos, Neg, Not, AddressOf, Deref,
	PreInc, PreDec,
	PostInc, PostDec,
};

struct UnaryExpr : Expr{
	UnaryOp op;
	std::unique_ptr<Expr> child;	
};

struct CallExpr : Expr{
	std::unique_ptr<Expr> func;
	std::vector<std::unique_ptr<Expr>> param;
};

struct IndexExpr : Expr{
	std::unique_ptr<Expr> index;
	std::unique_ptr<Expr> arr;
};

enum class AccessKind {Dot, Arrow};

struct AccessExpr : Expr{
	std::unique_ptr<Expr> object;
	std::string field;
	AccessKind kind; 
};

struct CastExpr : Expr{
	std::unique_ptr<Type> target;
	std::unique_ptr<Expr> expr;
};

struct IntLiteral : Expr{
	int value;
};

struct FloatLiteral : Expr{
	double value;
};

struct StringLiteral : Expr{
	std::string value;
};

struct Identifier : Expr{
	std::string name;
};

// Type

enum class BuiltinTypes{
	Int, Float, Char, Void
};

struct BuiltinType : Type{
	BuiltinTypes type;
	BuiltinType(BuiltinTypes bt) : type(bt) {}
};

struct PointerType : Type{
	std::unique_ptr<Type> base;
};

struct ArrayType : Type{
	std::unique_ptr<Type> elemType;
	std::size_t size;
};

struct FuncType : Type{
	std::unique_ptr<Type> returnType;
    std::vector<std::unique_ptr<Type>> params;
};




// иногда знак "=" может быть частью выражения, а иногда чисто как инструкцию
// Почему не используют "++" потому что это инструкция, а не вырежение, оно мутирует
// По наполнению зависит от нас, от студентов, многие из узлов содержат указатели на другие узлы 
// decl = func, class, typedef, tampletes, alies
