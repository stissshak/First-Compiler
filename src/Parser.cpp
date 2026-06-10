// MPL/src/Parser.cpp

#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include <algorithm>

#include "Parser.hpp"

//----------------------------------------
// string_view to int/float

int svtoi(std::string_view str){
	int res = 0;
	std::size_t len = str.length();
	for(std::size_t i = 0; i < len; ++i){
		res *= 10;
		res += str[i]  - '0';
	}
	return res;
}

double svtod(std::string_view str){
	std::size_t dot = str.find('.');
	if(dot == std::string_view::npos) return static_cast<double>(svtoi(str));
	double f = static_cast<double>(svtoi(str.substr(0, dot)));
	double s = static_cast<double>(svtoi(str.substr(dot + 1)));
	double div = 1.0;
	for(std::size_t i = dot + 1; i < str.length(); ++i) div *= 10.0;
	return f + s / div;
}

//----------------------------------------
// tools

bool Parser::isEnd() const{
	return peek().kind == TokenKind::Eof;
}

const Token& Parser::peek(std::size_t shift) const{
	if(pos + shift >= len) return raw[len-1];
	return raw[pos + shift];
}

const Token& Parser::take(){ return raw[pos++];}

bool Parser::match(TokenKind type){
	if(peek().kind == type){
		take(); return true;
	}
	return false;
}

void Parser::fail(const std::string& msg) const{
	throw std::runtime_error(smap.where(peek().offset, buffer) + ": " + msg);
}

void Parser::expect(TokenKind kind){
	if(peek().kind != kind){
		fail("Unexpected token");
	}
	take();
}

//----------------------------------------
// main func

std::unique_ptr<TranslationUnit> Parser::parse(){
	auto trUnit = std::make_unique<TranslationUnit>();
	trUnit->decls = std::vector<std::unique_ptr<Decl>>();
	while(!isEnd()){
		trUnit->decls.push_back(parseDecl());
	}
	return trUnit;
}

//----------------------------------------

bool Parser::isType(const Token& t){
	switch(t.kind){
		case TokenKind::IntK:
		case TokenKind::FloatK:
		case TokenKind::CharK:
		case TokenKind::VoidK:
		case TokenKind::BoolK:
			return true;
		case TokenKind::Identifier:
			if(std::find(userTypes.begin(), userTypes.end(), t.data) == userTypes.end()){
				return false;
			}
			return true;
		default:
			return false;
	}
}

std::unique_ptr<Type> Parser::parseBaseType() {
    switch(peek().kind){
        case TokenKind::IntK:
            take();
            return std::make_unique<BuiltinType>(BuiltinTypes::Int);
        case TokenKind::FloatK:
            take();
            return std::make_unique<BuiltinType>(BuiltinTypes::Float);
		case TokenKind::CharK:
			take();
			return std::make_unique<BuiltinType>(BuiltinTypes::Char);
		case TokenKind::VoidK:
			take();
			return std::make_unique<BuiltinType>(BuiltinTypes::Void);
		case TokenKind::BoolK:
			take();
			return std::make_unique<BuiltinType>(BuiltinTypes::Bool);
		
		default:
			if (peek().kind != TokenKind::Identifier)
        		fail("Expected type name");

			auto t = take();
            return std::make_unique<BuiltinType>(BuiltinTypes::Custom , t.data);
			
    }
}

std::unique_ptr<Type> Parser::parseType(){
	auto type = parseBaseType();
	while(match(TokenKind::Star)){
		auto p = std::make_unique<PointerType>();
		p->base = std::move(type);
		type = std::move(p);
	}
	return type;
}

//----------------------------------------
// declaraion

std::unique_ptr<Decl> Parser::parseDecl(){
	if(match(TokenKind::Struct)){
		return parseStruct();
	}
	if(isType(peek())){
		int i = 1;
		while(!isEnd() && peek(i).kind == TokenKind::Star) ++i;
		if(peek(i).kind != TokenKind::Identifier) fail("Expected identifier");
		if(peek(i+1).kind == TokenKind::LPar) return parseFunction();
		return parseVarDecl();
	}
	fail("Expected declaration");
}


std::unique_ptr<VarDecl> Parser::parseParam(){
    auto type = parseType();
    
	if(peek().kind != TokenKind::Identifier) fail("Exepted identifier");
	auto name = take();

    auto var = std::make_unique<VarDecl>();
    var->offset = name.offset;
    var->type = std::move(type);
    var->name = name.data;
    return var;
}


std::unique_ptr<Decl> Parser::parseStruct(){
	if(peek().kind != TokenKind::Identifier)
    	fail("Expected identifier");
	auto& nameTok = take();
	auto name = nameTok.data;
	expect(TokenKind::LBlock);
	auto fields = std::vector<std::unique_ptr<VarDecl>>();
	while(!isEnd() && peek().kind != TokenKind::RBlock){
		fields.push_back(parseParam());
		if(!match(TokenKind::Semicolon)) break; 
	}
	expect(TokenKind::RBlock);

	auto stct = std::make_unique<StructDecl>();
	stct->offset = nameTok.offset;
	stct->name = name;
	stct->fields = std::move(fields);
	userTypes.push_back(name);
	return stct;
}


std::unique_ptr<Decl> Parser::parseFunction(){
	auto type = parseType();

	if(peek().kind != TokenKind::Identifier)
    	fail("Expected identifier");
	auto& nameTok = take();
	auto name = nameTok.data;

	expect(TokenKind::LPar);
	auto args = std::vector<std::unique_ptr<VarDecl>>();
	bool variadic = false;

	while(!isEnd() && peek().kind != TokenKind::RPar){
		if(match(TokenKind::Ellipsis)){ variadic = true; break; } // must be last
		args.push_back(parseParam());
		if(!match(TokenKind::Comma)) break;
	}
	expect(TokenKind::RPar);


	auto func = std::make_unique<FuncDecl>();
	func->offset = nameTok.offset;
	func->returnType = std::move(type);
	func->name = name;
	func->params = std::move(args);
	func->variadic = variadic;
	if(match(TokenKind::Semicolon)) func->body = nullptr;
	else func->body = parseBlock();
	return func;
}

std::unique_ptr<Decl> Parser::parseVarDecl(){
	auto type = parseType();

	if (peek().kind != TokenKind::Identifier) fail("Expected type name");

	auto name = take();

	// TODO multi-dim, size as expr
	if(match(TokenKind::LBracket)){
		if(peek().kind != TokenKind::Int) fail("Expected array size");
		auto arr = std::make_unique<ArrayType>();
		arr->size = svtoi(take().data);
		arr->elemType = std::move(type);
		type = std::move(arr);
		expect(TokenKind::RBracket);
	}

	std::unique_ptr<Expr> expr = nullptr;
	if(match(TokenKind::Assign))
		expr = parseExpr();

	expect(TokenKind::Semicolon);
	auto var = std::make_unique<VarDecl>();
	var->offset = name.offset;
	var->type = std::move(type);
	var->name = name.data;
	var->init = std::move(expr);
	return var;
}

//----------------------------------------
// statments

std::unique_ptr<Stmt> Parser::parseStmt(){
	std::size_t off = peek().offset;
	std::unique_ptr<Stmt> s;
	switch(peek().kind){
		case TokenKind::If: s = parseIf(); break;
		case TokenKind::While: s = parseWhile(); break;
		case TokenKind::For: s = parseFor(); break;
		case TokenKind::LBlock: s = parseBlock(); break;
		case TokenKind::Return:   s = parseReturn(); break;
        case TokenKind::Break:    { take(); expect(TokenKind::Semicolon); s = std::make_unique<BreakStmt>(); break; }
        case TokenKind::Continue: { take(); expect(TokenKind::Semicolon); s = std::make_unique<ContinueStmt>(); break; }
		default:
			if(isType(peek())){
				auto ds = std::make_unique<DeclStmt>();
                ds->decl = parseVarDecl();
                s = std::move(ds);
			}
			else s = parseExprStmt();
	}
	s->offset = off;
	return s;
}

std::unique_ptr<Stmt> Parser::parseBlock(){
	expect(TokenKind::LBlock);
	auto statements = std::vector<std::unique_ptr<Stmt>>();
	while(!isEnd() && peek().kind != TokenKind::RBlock){
		statements.push_back(parseStmt());
	}
	expect(TokenKind::RBlock);
	auto node = std::make_unique<BlockStmt>();
	node->statements = std::move(statements);
	return node;
}

std::unique_ptr<Stmt> Parser::parseIf(){
	take();

	expect(TokenKind::LPar);
	auto cond = parseExpr();
	expect(TokenKind::RPar);
	auto thenPart = parseStmt();

	// if else
	std::unique_ptr<Stmt> elsePart = nullptr;
	if(match(TokenKind::Else)){
		elsePart = parseStmt();
	}

	auto node = std::make_unique<IfStmt>();
	node->cond = std::move(cond);
	node->thenPart = std::move(thenPart);
	node->elsePart = std::move(elsePart);

	return node;
}

std::unique_ptr<Stmt> Parser::parseWhile(){
	take();
	expect(TokenKind::LPar);
	auto cond = parseExpr();
	expect(TokenKind::RPar);
	auto body = parseStmt();

	auto node = std::make_unique<WhileStmt>();
	node->cond = std::move(cond);
	node->body = std::move(body);

	return node;
	
}

std::unique_ptr<Stmt> Parser::parseFor(){
	take();
	expect(TokenKind::LPar);

	std::unique_ptr<Decl> initDecl;
    std::unique_ptr<Stmt> initStmt;

	if(peek().kind == TokenKind::Semicolon){
		take();
	} else if(isType(peek())){
		initDecl = parseVarDecl();
	} else{
		initStmt = parseExprStmt();
	}

	std::unique_ptr<Expr> cond = nullptr;
	if(peek().kind != TokenKind::Semicolon)
		cond = parseExpr();
	expect(TokenKind::Semicolon);

	std::unique_ptr<Expr> incr = nullptr;
	if(peek().kind != TokenKind::RPar)
    	incr = parseExpr();
	expect(TokenKind::RPar);
	auto body = parseStmt();
		
	auto node = std::make_unique<ForStmt>();
	node->initDecl = std::move(initDecl);
	node->initStmt = std::move(initStmt);
	node->cond = std::move(cond);
	node->incr = std::move(incr);
	node->body = std::move(body);

	return node;
}

std::unique_ptr<Stmt> Parser::parseReturn(){
	take();
	std::unique_ptr<Expr> value = nullptr;
    if(peek().kind != TokenKind::Semicolon)
        value = parseExpr();
    expect(TokenKind::Semicolon);
    auto node = std::make_unique<ReturnStmt>();
    node->value = std::move(value);
    return node;
}

std::unique_ptr<Stmt> Parser::parseExprStmt(){
	auto exprStmt = std::make_unique<ExprStmt>();
	exprStmt->expr = parseExpr();
	expect(TokenKind::Semicolon);
	return exprStmt;
}

//----------------------------------------
// exprs

std::unique_ptr<Expr> Parser::parseExpr(){
	return parseBinary();
}

struct OpInfo{
	int prec;
	bool rightAcc;
	
	OpInfo(const Token& tok){
		switch(tok.kind){
			case TokenKind::Assign:      // =
			case TokenKind::PlusAssign:  // +=
			case TokenKind::MinusAssign: // -=
			case TokenKind::StarAssign: case TokenKind::SlashAssign: case TokenKind::PercAssign:
			case TokenKind::AmperAssign: case TokenKind::PipeAssign: case TokenKind::CarretAssign:
			case TokenKind::LessLessAssign: case TokenKind::GreatGreatAssign:
			prec = 0; rightAcc = true;
				break;       
			case TokenKind::PipePipe:          // ||
				prec = 1; rightAcc = false;
				break;      	
			case TokenKind::AmperAmper:         // &&
				prec = 2; rightAcc = false;
				break;
			case TokenKind::Pipe:
				prec = 3; rightAcc = false;
				break;
			case TokenKind::Carret:
				prec = 4; rightAcc = false;
				break;
			case TokenKind::Amper:
				prec = 5; rightAcc = false;
				break;
			case TokenKind::AssignAssign:
			case TokenKind::ExclAssign:
				prec = 6; rightAcc = false;
				break;
			case TokenKind::Less:
			case TokenKind::Great:
			case TokenKind::LessAssign:
			case TokenKind::GreatAssign:
				prec = 7; rightAcc = false;
				break;
			case TokenKind::LessLess: case TokenKind::GreatGreat:
                prec = 8; rightAcc = false;
				break;
			case TokenKind::Plus:
			case TokenKind::Minus:
				prec = 9; rightAcc = false; break;
			case TokenKind::Star:
			case TokenKind::Slash:
			case TokenKind::Perc:
				prec = 10; rightAcc = false; break;
			default:
				prec = -1; rightAcc = false;
				break;
		}
	}
};


BinaryOp tokenToBinOp(const Token& tok) {
	switch(tok.kind){
		case TokenKind::Assign: return BinaryOp::Assign;
		case TokenKind::PlusAssign: return BinaryOp::AddAssign;
		case TokenKind::MinusAssign: return BinaryOp::MinusAssign;
		case TokenKind::Plus: return BinaryOp::Add;
		case TokenKind::Minus: return BinaryOp::Sub;
		case TokenKind::Star: return BinaryOp::Mul;
		case TokenKind::Slash: return BinaryOp::Div;
		case TokenKind::Perc: return BinaryOp::Mod;
		case TokenKind::AssignAssign: return BinaryOp::Equal;
		case TokenKind::ExclAssign: return BinaryOp::NotEqual;
		case TokenKind::Less: return BinaryOp::Less;
		case TokenKind::Great: return BinaryOp::Greater;
		case TokenKind::LessAssign: return BinaryOp::LessEqual;
		case TokenKind::GreatAssign: return BinaryOp::GreaterEqual;
		case TokenKind::AmperAmper: return BinaryOp::And;
		case TokenKind::PipePipe: return BinaryOp::Or;
		case TokenKind::StarAssign: return BinaryOp::MulAssign;
		case TokenKind::SlashAssign: return BinaryOp::DivAssign;
		case TokenKind::PercAssign: return BinaryOp::ModAssign;
		case TokenKind::Amper: return BinaryOp::BitAnd;
		case TokenKind::Pipe: return BinaryOp::BitOr;
		case TokenKind::Carret: return BinaryOp::BitXor;
		case TokenKind::LessLess: return BinaryOp::Shl;
		case TokenKind::GreatGreat: return BinaryOp::Shr;
		case TokenKind::AmperAssign: return BinaryOp::BitAndAssign;
		case TokenKind::PipeAssign: return BinaryOp::BitOrAssign;
		case TokenKind::CarretAssign: return BinaryOp::BitXorAssign;
		case TokenKind::LessLessAssign: return BinaryOp::ShlAssign;
		case TokenKind::GreatGreatAssign: return BinaryOp::ShrAssign;
		default: throw std::runtime_error("Unexpected binary operator");
	}
}

std::unique_ptr<Expr> Parser::parseBinary(int minPrior){
	auto l = parseUnary();

	while(true){
		auto op = peek();
		auto info = OpInfo(op);

		if(info.prec < minPrior) break;

		take();
		
		int nextMinPrec = info.prec;
		if(!info.rightAcc) nextMinPrec += 1;

		auto r = parseBinary(nextMinPrec);

		auto node = std::make_unique<BinaryExpr>();
		node->offset = op.offset;
		node->left = std::move(l);
		node->right = std::move(r);
		node->op = tokenToBinOp(op);
		l = std::move(node);
	}	

	return l;
}

std::unique_ptr<Expr> Parser::parseUnary(){
	if(peek().kind == TokenKind::LPar && isType(peek(1))){
		auto off = peek().offset;
        take();              
        auto type = parseType();
        expect(TokenKind::RPar);
        auto inner = parseUnary();
		auto node = std::make_unique<CastExpr>();
		node->offset = off;
        node->target = std::move(type);
        node->expr = std::move(inner);
        return node;

	}
	const Token &tok = peek();

	std::unique_ptr<UnaryExpr> node = nullptr;
	switch(tok.kind){
		case TokenKind::Minus: // -
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::Neg;
			node->child = parseUnary();
			return node;
		case TokenKind::Plus: // +
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::Pos;
			node->child = parseUnary();
			return node;
		case TokenKind::Star: // *
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::Deref;
			node->child = parseUnary();
			return node;
		case TokenKind::Excl:   // !
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::Not;
			node->child = parseUnary();
			return node;
		case TokenKind::Amper:  // &
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::AddressOf;
			node->child = parseUnary();
			return node;
		case TokenKind::PlusPlus:   // ++
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::PreInc;
			node->child = parseUnary();
			return node;
		case TokenKind::MinusMinus: // --
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::PreDec;
			node->child = parseUnary();
			return node;
		case TokenKind::Tilda: // ~
			take();
			node = std::make_unique<UnaryExpr>();
			node->offset = tok.offset;
			node->op = UnaryOp::BitNot;
			node->child = parseUnary();
			return node;
		default:
			return parsePostfix();	
	}
}

std::unique_ptr<Expr> Parser::parsePostfix(){
	std::size_t primOff = peek().offset;
	auto expr = parsePrimary();
	expr->offset = primOff;
	while(true) {
        const Token &tok = peek();

        switch(tok.kind) {
            case TokenKind::LPar: 
			{
                take();

                auto call = std::make_unique<CallExpr>();
                call->offset = tok.offset;
                call->func = std::move(expr);

                if(peek().kind != TokenKind::RPar) {
                    while(true) {
                        call->param.push_back(parseExpr());
                        if(!match(TokenKind::Comma)) break;
                    }
                }

                expect(TokenKind::RPar);

                expr = std::move(call);
                break;
            }
            case TokenKind::LBracket: 
			{
                take();

                auto index = std::make_unique<IndexExpr>();
                index->offset = tok.offset;
                index->arr = std::move(expr);
                index->index = parseExpr();

                expect(TokenKind::RBracket);

                expr = std::move(index);
                break;
            }
            case TokenKind::Dot: 
			{
                take();

                auto access = std::make_unique<AccessExpr>();
                access->offset = tok.offset;
                access->object = std::move(expr);
                access->kind = AccessKind::Dot;

                if(peek().kind != TokenKind::Identifier)
                    fail("Expected field after '.'");

                access->field = take().data;

                expr = std::move(access);
                break;
            }
            case TokenKind::Arrow: 
			{
                take();

                auto access = std::make_unique<AccessExpr>();
                access->offset = tok.offset;
                access->object = std::move(expr);
                access->kind = AccessKind::Arrow;

                if(peek().kind != TokenKind::Identifier)
                    fail("Expected field after '->'");

                access->field = take().data;

                expr = std::move(access);
                break;
            }
			case TokenKind::PlusPlus:
    		{
					take();
					auto u = std::make_unique<UnaryExpr>();
					u->offset = tok.offset;
					u->op = UnaryOp::PostInc;
					u->child = std::move(expr);
					expr = std::move(u);
					break;
			}
			case TokenKind::MinusMinus:
			{
				take();
				auto u = std::make_unique<UnaryExpr>();
				u->op = UnaryOp::PostDec;
				u->child = std::move(expr);
				expr = std::move(u);
				break;
			}
            default:
                return expr;
        }
    }
}

std::unique_ptr<Expr> Parser::parsePrimary(){
	const Token &tok = peek();

	switch(tok.kind){
		case TokenKind::Int:
		{
			take();
			auto node = std::make_unique<IntLiteral>();
			node->value = svtoi(tok.data);
			return node;
		}
		case TokenKind::Float:
		{    
			take();
			auto node = std::make_unique<FloatLiteral>();
			node->value = svtod(tok.data);
			return node;
		}
		case TokenKind::Char:
		{
			take();
			auto node = std::make_unique<CharLiteral>();
			node->value = tok.data[1];
			return node;
		}
		case TokenKind::True:
		{
			take();
			auto node = std::make_unique<BoolLiteral>();
			node->value = true;
			return node;
		}
		case TokenKind::False:
		{
			take();
			auto node = std::make_unique<BoolLiteral>();
			node->value = false;
			return node;
		}
		case TokenKind::String:
		{
			take();
			auto node = std::make_unique<StringLiteral>();
			node->value = tok.data.substr(1, tok.data.size() - 2);
			return node;
		}
		case TokenKind::Identifier:
		{	
			take();
			auto node = std::make_unique<Identifier>();
			node->name = tok.data;
			return node;
		}
		case TokenKind::LPar:
		{
			take();
			auto expr = parseExpr();
			expect(TokenKind::RPar);
			return expr;
		}
			default:
			fail("Unexpected token");
	}
}











