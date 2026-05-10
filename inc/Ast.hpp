// MPL/inc/Ast.hpp

#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <vector>


struct AstVisitor;

#define ACCEPT void accept(AstVisitor& v) override;

struct Node{
	virtual ~Node() = default;
	virtual void accept(AstVisitor& v) = 0;
};

struct Decl : Node {};
struct Stmt : Node {};
struct Expr : Node {};
struct Type : Node {};

// Target
struct TranslationUnit : Node{
	std::vector<std::unique_ptr<Decl>> decls;
	
	ACCEPT
};

// Decl
struct VarDecl : Decl{
	std::unique_ptr<Type> type;
	std::string_view name;
	std::unique_ptr<Expr> init;
	
	ACCEPT
};

struct FuncDecl : Decl{
	std::unique_ptr<Type> returnType;
	std::string_view name;
	std::vector<std::unique_ptr<VarDecl>> params;
	std::unique_ptr<Stmt> body;
	
	ACCEPT
};

struct StructDecl : Decl{
	std::string_view name;
	std::vector<std::unique_ptr<VarDecl>> fields;

	ACCEPT
};

// Stmt
struct BlockStmt : Stmt{
	std::vector<std::unique_ptr<Stmt>> statements;

	ACCEPT
};

struct ExprStmt : Stmt{
	std::unique_ptr<Expr> expr;

	ACCEPT
};

struct DeclStmt : Stmt{
    std::unique_ptr<Decl> decl;

	ACCEPT
};

struct IfStmt : Stmt{
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Stmt> thenPart;
	std::unique_ptr<Stmt> elsePart;

	ACCEPT
};

struct WhileStmt : Stmt{
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Stmt> body;

	ACCEPT
};

struct ForStmt : Stmt{
	std::unique_ptr<Node> init; // Variant<Expr, Decl>
	std::unique_ptr<Expr> cond;
	std::unique_ptr<Expr> incr;
	std::unique_ptr<Stmt> body;

	ACCEPT
};

struct ReturnStmt : Stmt{
	std::unique_ptr<Expr> value; 

	ACCEPT
};

struct BreakStmt : Stmt{ ACCEPT };
struct ContinueStmt : Stmt{ ACCEPT };



// Expr
// Operations

enum class BinaryOp{
	Add, Sub, Mul, Div, Mod,
	Less, Greater, Equal, NotEqual,
	LessEqual, GreaterEqual,
	And, Or,
	Assign,	AddAssign, MinusAssign 

};

struct BinaryExpr : Expr{
	BinaryOp op;
	std::unique_ptr<Expr> left;
	std::unique_ptr<Expr> right;

	ACCEPT
};

enum class UnaryOp{
	Pos, Neg, Not, AddressOf, Deref,
	PreInc, PreDec,
	PostInc, PostDec,
};

struct UnaryExpr : Expr{
	UnaryOp op;
	std::unique_ptr<Expr> child;	

	ACCEPT
};

struct CallExpr : Expr{
	std::unique_ptr<Expr> func;
	std::vector<std::unique_ptr<Expr>> param;

	ACCEPT
};

struct IndexExpr : Expr{
	std::unique_ptr<Expr> index;
	std::unique_ptr<Expr> arr;

	ACCEPT
};

enum class AccessKind {Dot, Arrow};

struct AccessExpr : Expr{
	std::unique_ptr<Expr> object;
	std::string_view field;
	AccessKind kind; 

	ACCEPT
};

struct CastExpr : Expr{
	std::unique_ptr<Type> target;
	std::unique_ptr<Expr> expr;

	//ACCEPT
};

struct IntLiteral : Expr{
	int value;

	ACCEPT
};

struct FloatLiteral : Expr{
	double value;

	ACCEPT
};

struct CharLiteral : Expr{
	char value;

	ACCEPT
};

struct StringLiteral : Expr{
	std::string_view value;

	ACCEPT
};

struct Identifier : Expr{
	std::string_view name;

	ACCEPT
};

// Type

enum class BuiltinTypes{
	Int, Float, Char, Void, Custom
};

struct BuiltinType : Type{
	BuiltinTypes type;
	std::string_view name;
	BuiltinType(BuiltinTypes bt) : type(bt) {}
	BuiltinType(BuiltinTypes bt, std::string_view name) : type(bt), name(name) {}

	ACCEPT
};

struct PointerType : Type{
	std::unique_ptr<Type> base;

	ACCEPT
};

struct ArrayType : Type{
	std::unique_ptr<Type> elemType;
	std::size_t size;

	// ACCEPT
};

struct FuncType : Type{
	std::unique_ptr<Type> returnType;
    std::vector<std::unique_ptr<Type>> params;

	// ACCEPT
};




// иногда знак "=" может быть частью выражения, а иногда чисто как инструкцию
// Почему не используют "++" потому что это инструкция, а не вырежение, оно мутирует
// По наполнению зависит от нас, от студентов, многие из узлов содержат указатели на другие узлы 
// decl = func, class, typedef, tampletes, alies

