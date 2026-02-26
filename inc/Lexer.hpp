// /inc/Lexer.hpp

#pragma once

#include <Token.hpp>

#include <vector>
#include <string>

class Lexer{
private:
    std::vector<Token> tokens;
    std::size_t size;
public:
    void tokenize(const &std::string input);
    const vector<Token> &get_tokens() const;
private:
    Token &extract(const &std::string input);
};
