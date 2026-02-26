// /inc/Token.hpp

#pragma once

#include <string>
#include <string_view>

namespace tok{ 
    enum TokenKind : unsigned short{ 
    #define TOK(ID, TEXT) ID, 
    #include "inc/TokenKind.hpp" 
    }; 
}

struct Token{
    std::string_view;       // Place of token and size from input
    tok::TokenKind kind;    // Type of token
};


