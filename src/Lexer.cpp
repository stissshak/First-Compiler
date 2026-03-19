// /src/Lexer.cpp

#include "Lexer.hpp"

#include <cctype>

bool Lexer::is_end() const{
    return pos >= len;
}

bool Lexer::start_num() const{
    return std::isdigit(peek()) || peek() == '.';
}

char Lexer::peek() const{
    return is_end() ? '\0' : raw[pos];
}

char Lexer::take(){
    return raw[pos++];
}

std::vector<Token> Lexer::tokenize(){
    std::vector<Token> res;
    while(!is_end()){
        res.push_back(extract());
    }
    return res;
}

Token Lexer::extract(){
    while(!is_end() && std::isspace(static_cast<unsigned char>(peek()))) ++pos;
    if(is_end()){
        return Token{TokenKind::Eof, {}};
    }
    if(peek() == '"') return extract_str();
    if(metachars.contains(peek())) return extract_op();
    if(start_num()) return extract_num();
    return extract_word();
}

Token Lexer::extract_str(){
    std::size_t start = pos;
    take();

    while(!is_end() && peek() != '"'){
        if(peek() == '\\'){
            take();
            if(!is_end()) take();
        }
        else take();
    }

    if(is_end()) return Token{TokenKind::Invalid, raw.substr(start, pos - start)};
    take();
    std::string_view str = raw.substr(start, pos - start);
    return Token{TokenKind::String, str};
}

Token Lexer::extract_op(){
    auto two = raw.substr(pos, 2);
    if(auto it = ops.find(two); it != ops.end()){
        pos += 2;
        return Token{it->second, two};
    }
    auto one = raw.substr(pos, 1);
    if(auto it = ops.find(one); it != ops.end()){
        pos += 1;
        return Token{it->second, one};
    }
    return Token{TokenKind::Invalid, {}};
}

Token Lexer::extract_num(){
    std::size_t start = pos;
    bool dot = false;

    if(peek() == '.' && !std::isdigit(raw[pos + 1])) return extract_op();
    while(!is_end()){
        char c = peek();

        if(std::isdigit(c)) take();
        else if(c == '.' && !dot){
            dot = true;
            take();
        }
        else break;
    }

    std::string_view num = raw.substr(start, pos - start);
    return dot ? Token{TokenKind::Float, num} : Token{TokenKind::Int, num};
}

Token Lexer::extract_word(){
    std::size_t start = pos;
    
    while(!is_end() &&!std::isspace(peek()) && !metachars.contains(peek())){
        take();
    }
    
    std::string_view word = raw.substr(start, pos - start);
    return Token{TokenKind::Word, word};
}

