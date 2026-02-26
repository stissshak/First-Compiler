// /src/Lexer.cpp

#include <Lexer.hpp>

Token &Lexer::extract(const &std::string input){
    
}

void Lexer::tokenize(const &std::string input){
    Token t = extract(input);
    tokens.push_back(t);
}
