// /inc/Token.hpp

#pragma once

#include <string>
#include <string_view>

enum class TokenKind{
    Int,
    Float,
    Identifier,
    String,

    Plus,       // +
    PlusPlus,   // ++
    Minus,      // -
    MinusMinus, // --
    Star,       // *
    Slash,      // /
    Perc,       // %
    PlusAssign, // +=
    MinusAssign,// -=
    SlashAssign,// /=
    PercAssign, // %=
    
    Excl,       // !
    Tilda,      // ~
    LessLess,   // <<
    GreatGreat, // >>
    Amper,      // &
    Pipe,       // |
    Carret,     // ^
    AmperAssign,// &=
    PipeAssign, // |=

    AmperAmper,     // &&
    PipePipe,       // ||
    Less,           // <
    Great,          // >
    LessAssign,     // <=
    GreatAssign,    // >=
    AssignAssign,   // ==
    ExclAssign,     // !=

    Assign,     // =
    Comma,      // ,
    Dot,        // .
    Arrow,      // ->
    Question,   // ?
    Colon,      // :
    Semicolon,  // ;
    LPar,       // (
    RPar,       // )
    LBlock,     // {
    RBlock,     // }
    LBracket,   // [
    RBracket,   // ]

    If,
    Else,
    For,
    While,
    Switch,
    Case,
    Return,

    IntK,
    FloatK,
    CharK,
    VoidK,
    

    Hash,       // #
    Invalid,
    Eof
};

struct Token{ 
    TokenKind kind;         // Type of token
    std::string_view data;  // Place of token and size from input
};


