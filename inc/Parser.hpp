// MPL/inc/Parser.hpp

#pragma once

#include <vector>
#include <string>

#include "Token.hpp"
#include "Ast.hpp"

class Parser{
public:
    Parser(const std::vector<Token>& raw) : raw(raw), pos(0), len(raw.size()) {}
    std::unique_ptr<TranslationUnit> parse();
private:
    bool isEnd() const;
    const Token& peek(std::size_t shift=0) const;
    const Token& take();
	bool match(TokenKind kind);
	void except(TokenKind kind);

	// declarations
	std::unique_ptr<Decl> parseDecl();
	std::unique_ptr<Decl> parseStruct();
	std::unique_ptr<VarDecl> parseParam();
	std::unique_ptr<Decl> parseFunction();
	std::unique_ptr<Decl> parseVarDecl();
	
	// statements
	std::unique_ptr<Stmt> parseStmt();
	std::unique_ptr<Stmt> parseBlock();
	std::unique_ptr<Stmt> parseIf();
	std::unique_ptr<Stmt> parseWhile();
	std::unique_ptr<Stmt> parseFor();
	std::unique_ptr<Stmt> parseReturn();
	std::unique_ptr<Stmt> parseExprStmt();
	
	// expressions
	std::unique_ptr<Expr> parseExpr();
	std::unique_ptr<Expr> parseBinary(int minPrior=0);
	std::unique_ptr<Expr> parseUnary();
	std::unique_ptr<Expr> parsePostfix();
	std::unique_ptr<Expr> parsePrimary();
	
	// types
	bool isType(Token t);
	std::unique_ptr<Type> parseType();

	// data
    const std::vector<Token>& raw;
    std::size_t pos;
    std::size_t len;

	std::vector<std::string_view> names;
};
