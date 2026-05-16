// /inc/Lexer.hpp

#pragma once

#include "Token.hpp"

#include <vector>
#include <string>
#include <unordered_map>

class Lexer{
public:
    Lexer(std::string_view str) : raw(str), pos(0), len(str.length()){}
    std::vector<Token> tokenize();
private:
    bool is_end() const;
    bool start_num() const;
    char peek() const;
    char take();
    void skip();
    Token extract();
    Token extract_str();
    Token extract_char();
    Token extract_op();
    Token extract_num();
    Token extract_word();

    std::string_view raw;
    std::size_t pos;
    std::size_t len;
};

static constexpr std::string_view metachars = "+-*/%=!~<>&|^,.-?;:(){}[]";

static const std::unordered_map<std::string_view, TokenKind> ops = {
    {"++", TokenKind::PlusPlus},
    {"+=", TokenKind::PlusAssign},
    {"+",  TokenKind::Plus},

    {"--", TokenKind::MinusMinus},
    {"-=", TokenKind::MinusAssign},
    {"->", TokenKind::Arrow},
    {"-",  TokenKind::Minus},

    {"<<=", TokenKind::LessLessAssign},
    {"<<", TokenKind::LessLess},
    {"<=", TokenKind::LessAssign},
    {"<",  TokenKind::Less},

    {">>=", TokenKind::GreatGreatAssign},
    {">>", TokenKind::GreatGreat},
    {">=", TokenKind::GreatAssign},
    {">",  TokenKind::Great},

    {"==", TokenKind::AssignAssign},
    {"=",  TokenKind::Assign},

    {"!=", TokenKind::ExclAssign},
    {"!",  TokenKind::Excl},

    {"&&", TokenKind::AmperAmper},
    {"&=", TokenKind::AmperAssign},
    {"&",  TokenKind::Amper},

    {"||", TokenKind::PipePipe},
    {"|=", TokenKind::PipeAssign},
    {"|",  TokenKind::Pipe},

    {"^=", TokenKind::CarretAssign},
    {"^",  TokenKind::Carret},
    {"~",  TokenKind::Tilda},

    {"*",  TokenKind::Star},
    {"*=", TokenKind::StarAssign},
    {"/=", TokenKind::SlashAssign},
    {"/",  TokenKind::Slash},
    {"%",  TokenKind::Perc},
    {"%=", TokenKind::PercAssign},

    {",", TokenKind::Comma},
    {".", TokenKind::Dot},
    {"?", TokenKind::Question},
    {":", TokenKind::Colon},
    {";", TokenKind::Semicolon},
    {"(", TokenKind::LPar},
    {")", TokenKind::RPar},
    {"{", TokenKind::LBlock},
    {"}", TokenKind::RBlock},
    {"[", TokenKind::LBracket},
    {"]", TokenKind::RBracket},
};

static const std::unordered_map<std::string_view, TokenKind> keys = {
    {"sizeof", TokenKind::Sizeof},
    {"typedef", TokenKind::Typedef},
    {"const", TokenKind::Const},
    {"static", TokenKind::Static},
    
    {"if", TokenKind::If},
    {"else", TokenKind::Else},

    {"for",  TokenKind::For},
    {"do", TokenKind::Do},
    {"while", TokenKind::While},
    {"switch", TokenKind::Switch},
    {"case", TokenKind::Case},
    {"default", TokenKind::Default},
    {"return", TokenKind::Return},
    {"break", TokenKind::Break},
    {"continue", TokenKind::Continue},

    {"int", TokenKind::IntK},
    {"float", TokenKind::FloatK},
    {"char", TokenKind::CharK},
    {"void", TokenKind::VoidK},

    {"struct", TokenKind::Struct},
    {"enum", TokenKind::Enum},
    {"union", TokenKind::Union}
};
