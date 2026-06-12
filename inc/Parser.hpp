// MPL/inc/Parser.hpp

#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "Token.hpp"
#include "Ast.hpp"
#include "SourceMap.hpp"

class Parser{
public:
    Parser(const std::vector<Token>& raw, const SourceMap& smap, const std::string& buffer)
        : raw(raw), pos(0), len(raw.size()), smap(smap), buffer(buffer) {}
    std::unique_ptr<TranslationUnit> parse();
private:
    bool isEnd() const;
    const Token& peek(std::size_t shift=0) const;
    const Token& take();
	bool match(TokenKind kind);
	void expect(TokenKind kind);
	[[noreturn]] void fail(const std::string& msg) const;

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
	bool isType(const Token& t);
	std::unique_ptr<Type> parseBaseType();
	std::unique_ptr<Type> parseType();

	// data
    const std::vector<Token>& raw;
    std::size_t pos;
    std::size_t len;
    const SourceMap& smap;
    const std::string& buffer;

	std::vector<std::string_view> userTypes;
	std::unordered_map<std::string_view, std::unique_ptr<Type>> aliases;   // typedef, parse-time only
};
