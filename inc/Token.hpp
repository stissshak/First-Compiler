// /inc/Token.hpp

#pragma once

#include <string>
#include <string_view>

enum class TokenKind{
    Int,
    Float,
    Char,
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
    StarAssign, // *=
    SlashAssign,// /=
    PercAssign, // %=
    
    Excl,               // !
    Tilda,              // ~
    LessLess,           // <<
    GreatGreat,         // >>
    Amper,              // &
    Pipe,               // |
    Carret,             // ^
    CarretAssign,       // ^=
    AmperAssign,        // &=
    PipeAssign,         // |=
    LessLessAssign,     // <<=
    GreatGreatAssign,   // >>=

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

    Do,
    Sizeof,
    Typedef,
    Const,
    Static,
    Union,
    Enum,
    If,
    Else,
    For,
    While,
    Switch,
    Case,
    Default,
    Return,
    Break,
    Continue,

    Struct,

    IntK,
    FloatK,
    CharK,
    VoidK,
    

    Invalid,
    Eof
};



struct Token{ 
    TokenKind kind;         // Type of token
    std::string_view data;  // Place of token and size from input
    std::size_t offset;           // Place in code
};


