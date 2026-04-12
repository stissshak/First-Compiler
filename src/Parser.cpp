// MPL/src/Parser.cpp

#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>

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

void Parser::except(TokenKind kind){
	if(peek().kind != kind){
		throw std::runtime_error("Unexcepted token");
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

bool isType(TokenKind kind){
	switch(kind){
		case TokenKind::IntK:
		case TokenKind::FloatK:
		case TokenKind::CharK:
		case TokenKind::VoidK:
			return true;
		default:
			return false;
	}
}

std::unique_ptr<Type> Parser::parseType() {
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
        default:
            throw std::runtime_error("Expected type");
    }
}

//----------------------------------------
// declaraion

std::unique_ptr<Decl> Parser::parseDecl(){
	if(isType(peek().kind)){

		int i = 1;
		while(!isEnd() && peek(i).kind == TokenKind::Star) ++i;
		if(peek(i).kind != TokenKind::Identifier) throw std::runtime_error("Exepter identifier");
		if(peek(i+1).kind == TokenKind::LPar) return parseFunction();
		return parseVarDecl();
	}
	throw std::runtime_error("Expected declaration");
}


std::unique_ptr<VarDecl> Parser::parseParam(){
    auto type = parseType();
    
	while(match(TokenKind::Star)){
		auto ptr = std::make_unique<PointerType>();
    	ptr->base = std::move(type);
    	type = std::move(ptr);
	}
	if(peek().kind != TokenKind::Identifier) throw std::runtime_error("Exepted identifier");
	auto name = take();

    auto var = std::make_unique<VarDecl>();
    var->type = std::move(type);
    var->name = name.data;
    return var;
}


std::unique_ptr<Decl> Parser::parseFunction(){
	auto type = parseType();

	while(match(TokenKind::Star)) {
		auto ptr = std::make_unique<PointerType>();
		ptr->base = std::move(type);
		type = std::move(ptr);
	}

	if(peek().kind != TokenKind::Identifier)
    	throw std::runtime_error("Expected identifier");
	auto name = take().data;
	
	except(TokenKind::LPar);
	auto args = std::vector<std::unique_ptr<VarDecl>>();
	
	while(!isEnd() && peek().kind != TokenKind::RPar){
		args.push_back(parseParam());
		if(!match(TokenKind::Comma)) break; 
	}
	except(TokenKind::RPar);

	auto body = parseBlock();

	auto func = std::make_unique<FuncDecl>();
	func->returnType = std::move(type);
	func->name = name;
	func->params = std::move(args);
	func->body = std::move(body);
	return func;
}

std::unique_ptr<Decl> Parser::parseVarDecl(){
	auto type = parseType();
	
	while(match(TokenKind::Star)){ 
        auto ptr = std::make_unique<PointerType>();
        ptr->base = std::move(type);
        type = std::move(ptr);
    }

	auto name = take();

	std::unique_ptr<Expr> expr = nullptr;
	if(match(TokenKind::Assign))
		expr = parseExpr();

	except(TokenKind::Semicolon);
	auto var = std::make_unique<VarDecl>();
	var->type = std::move(type);
	var->name = name.data;
	var->init = std::move(expr);
	return var;
}

//----------------------------------------
// statments

std::unique_ptr<Stmt> Parser::parseStmt(){
	switch(peek().kind){
		case TokenKind::If: return parseIf();
		case TokenKind::While: return parseWhile();
		case TokenKind::For: return parseFor();
		case TokenKind::LBlock: return parseBlock();
		case TokenKind::Return:   return parseReturn();
        case TokenKind::Break:    { take(); except(TokenKind::Semicolon); return std::make_unique<BreakStmt>(); }
        case TokenKind::Continue: { take(); except(TokenKind::Semicolon); return std::make_unique<ContinueStmt>(); }
		default: 
			if(isType(peek().kind)){
				auto ds = std::make_unique<DeclStmt>();
                ds->decl = parseVarDecl();
                return ds;
			}
			return parseExprStmt();
	}
}

std::unique_ptr<Stmt> Parser::parseBlock(){
	except(TokenKind::LBlock);
	auto statements = std::vector<std::unique_ptr<Stmt>>();
	while(!isEnd() && peek().kind != TokenKind::RBlock){
		statements.push_back(parseStmt());
	}
	except(TokenKind::RBlock);
	auto node = std::make_unique<BlockStmt>();
	node->statements = std::move(statements);
	return node;
}

std::unique_ptr<Stmt> Parser::parseIf(){
	take();

	except(TokenKind::LPar);
	auto cond = parseExpr();
	except(TokenKind::RPar);
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
	except(TokenKind::LPar);
	auto cond = parseExpr();
	except(TokenKind::RPar);
	auto body = parseStmt();

	auto node = std::make_unique<WhileStmt>();
	node->cond = std::move(cond);
	node->body = std::move(body);

	return node;
	
}

std::unique_ptr<Stmt> Parser::parseFor(){
	take();
	except(TokenKind::LPar);
	std::unique_ptr<Node> init;

	if(match(TokenKind::Semicolon)) {
		init = nullptr;
	} else {
		if(isType(peek().kind))
			init = parseVarDecl();
		else
			init = parseExprStmt();
	}

	std::unique_ptr<Expr> cond = nullptr;
	if(peek().kind != TokenKind::Semicolon)
		cond = parseExpr();
	except(TokenKind::Semicolon);

	std::unique_ptr<Expr> incr = nullptr;
	if(peek().kind != TokenKind::RPar)
    	incr = parseExpr();
	except(TokenKind::RPar);
	auto body = parseStmt();
		
	auto node = std::make_unique<ForStmt>();
	node->init = std::move(init);
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
    except(TokenKind::Semicolon);
    auto node = std::make_unique<ReturnStmt>();
    node->value = std::move(value);
    return node;
}

std::unique_ptr<Stmt> Parser::parseExprStmt(){
	auto exprStmt = std::make_unique<ExprStmt>();
	exprStmt->expr = parseExpr();
	except(TokenKind::Semicolon);
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
				prec = 0; rightAcc = true;
				break;       
			case TokenKind::PipePipe:          // ||
				prec = 1; rightAcc = false;
				break;      	
			case TokenKind::AmperAmper:         // &&
				prec = 2; rightAcc = false;
				break;
			case TokenKind::AssignAssign:
			case TokenKind::ExclAssign:
				prec = 3; rightAcc = false; break;
			case TokenKind::Less:
			case TokenKind::Great:
			case TokenKind::LessAssign:
			case TokenKind::GreatAssign:
				prec = 4; rightAcc = false; break;
			case TokenKind::Plus:
			case TokenKind::Minus:
				prec = 5; rightAcc = false; break;
			case TokenKind::Star:
			case TokenKind::Slash:
			case TokenKind::Perc:
				prec = 6; rightAcc = false; break;
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
		node->left = std::move(l);
		node->right = std::move(r);
		node->op = tokenToBinOp(op);
		l = std::move(node);
	}	

	return l;
}

std::unique_ptr<Expr> Parser::parseUnary(){
	const Token tok = peek();

	std::unique_ptr<UnaryExpr> node = nullptr;
	switch(tok.kind){
		case TokenKind::Minus: // -
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::Neg;
			node->child = parseUnary();
			return node;
		case TokenKind::Plus: // +
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::Pos;
			node->child = parseUnary();
			return node;
		case TokenKind::Star: // *
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::Deref;
			node->child = parseUnary();
			return node;
		case TokenKind::Excl:   // !
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::Not;
			node->child = parseUnary();
			return node;
		case TokenKind::Amper:  // &
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::AddressOf;
			node->child = parseUnary();
			return node;
		case TokenKind::PlusPlus:   // ++
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::PreInc;
			node->child = parseUnary();
			return node;
		case TokenKind::MinusMinus: // --
			take();
			node = std::make_unique<UnaryExpr>();
			node->op = UnaryOp::PreDec;
			node->child = parseUnary();
			return node;
		default:
			return parsePostfix();	
	}
}

std::unique_ptr<Expr> Parser::parsePostfix(){
	auto expr = parsePrimary();	
	while(true) {
        const Token tok = peek();

        switch(tok.kind) {
            case TokenKind::LPar: 
			{
                take();

                auto call = std::make_unique<CallExpr>();
                call->func = std::move(expr);

                if(peek().kind != TokenKind::RPar) {
                    while(true) {
                        call->param.push_back(parseExpr());
                        if(!match(TokenKind::Comma)) break;
                    }
                }

                except(TokenKind::RPar);

                expr = std::move(call);
                break;
            }
            case TokenKind::LBracket: 
			{
                take();

                auto index = std::make_unique<IndexExpr>();
                index->arr = std::move(expr);
                index->index = parseExpr();

                except(TokenKind::RBracket);

                expr = std::move(index);
                break;
            }
            case TokenKind::Dot: 
			{
                take();

                auto access = std::make_unique<AccessExpr>();
                access->object = std::move(expr);
                access->kind = AccessKind::Dot;

                if(peek().kind != TokenKind::Identifier)
                    throw std::runtime_error("Expected field after '.'");

                access->field = std::string(take().data);

                expr = std::move(access);
                break;
            }
            case TokenKind::Arrow: 
			{
                take();

                auto access = std::make_unique<AccessExpr>();
                access->object = std::move(expr);
                access->kind = AccessKind::Arrow;

                if(peek().kind != TokenKind::Identifier)
                    throw std::runtime_error("Expected field after '->'");

                access->field = std::string(take().data);

                expr = std::move(access);
                break;
            }
			case TokenKind::PlusPlus:
    		{
					take();
					auto u = std::make_unique<UnaryExpr>();
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
	const Token tok = peek();

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
		case TokenKind::String:
		{
			take();
			auto node = std::make_unique<StringLiteral>();
			node->value = std::string(tok.data);
			return node;
		}
		case TokenKind::Identifier:
		{	
			take();
			auto node = std::make_unique<Identifier>();
			node->name = std::string(tok.data);
			return node;
		}
		case TokenKind::LPar:
		{
			take();
			auto expr = parseExpr();
			except(TokenKind::RPar);
			return expr;
		}
			default:
			throw std::runtime_error("Unexpected token");
	}
}











