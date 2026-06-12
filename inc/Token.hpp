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
    True,
    False,

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
    Ellipsis,   // ...
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


    Sizeof,
    Typeid,
    Typedef,
    Const,
    Static,
    Return,

    If,
    Else,

    For,
    While,
    Do,

    Switch,
    Case,
    Default,
    Break,
    Continue,

    Struct,
    Union,
    Enum,
    Extern,

    IntK,
    FloatK,
    BoolK,
    CharK,
    VoidK,
    ByteK,
    ShortK,
    LongK,
    UIntK,
    

    Invalid,
    Eof
};



struct Token{ 
    TokenKind kind;         // Type of token
    std::string_view data;  // Place of token and size from input
    std::size_t offset = 0; // Place in code
};


