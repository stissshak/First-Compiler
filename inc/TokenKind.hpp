// MPL/inc/TokenKind.hpp

#ifndef TOK
#define TOK(ID, TEXT)
#endif

#ifndef KEYWORD
#define KEYWORD(ID, TEXT) TOK(ID, TEXT)
#endif

////////////////////////////////////

// Keywords
KEYWORD(Int32, "i32")
KEYWORD(Int64, "i64")
KEYWORD(Float, "f32")
KEYWORD(Double, "f64")
KEYWORD(Void, "void")

KEYWORD(Var, "let")
KEYWORD(Mut, "mut")
KEYWORD(Def, "fn")
KEYWORD(Struct, "struct")
KEYWORD(Class, "class")
KEYWORD(Impl, "impl")

KEYWORD(If, "if")
KEYWORD(Else, "else")
KEYWORD(Switch, "match")

KEYWORD(For, "for")
KEYWORD(While, "while")
KEYWORD(Loop, "loop")
KEYWORD(Return, "return")
KEYWORD(Break, "break")
KEYWORD(Continue, "continue")

/////////////////////////////////////


// types
TOK(Int32Number, "")
TOK(Int64Number, "")
TOK(FloatNumber, "")
TOK(DoubleNumber, "")
TOK(CharLiteral, "")
TOK(StringConstant, "")

// arithm
TOK(Plus, "+")
TOK(PlusPlus, "++")
TOK(Minus, "-")
TOK(MinusMinus, "--")
TOK(Mul, "*")
TOK(Div, "/")
TOK(Mod, "%")
TOK(PlusAssign, "+=")
TOK(MinusAssign, "-=")
TOK(MulAssign, "*=")
TOK(DivAssign, "/=")
TOK(ModAssign, "%=")


// byte
TOK(Not, "!")
TOK(Tilda, "~")
TOK(LShift, "<<")
TOK(RShift, ">>")
TOK(BitAnd, "&")
TOK(BitOr, "|")
TOK(BitXor, "^")
TOK(BitAndAssign, "&=")
TOK(BitOrAssign, "|=")

// logic
TOK(LogAnd, "&&")
TOK(LogOr, "||")
TOK(Less, "<")
TOK(Greater, ">")
TOK(LessEqual, "<=")
TOK(GreaterEqual, ">=")
TOK(Equal, "==")
TOK(NotEqual, "!=")

// ??
TOK(Assign, "=")
TOK(Comma, ",")
TOK(Dot, ".")
TOK(Arrow, "->")

// service
TOK(Question, "?")
TOK(Colon, ":")
TOK(Semicolon, ";")
TOK(OpenParen, "(")
TOK(CloseParen, ")")
TOK(BlockStart, "{")
TOK(BlockEnd, "}")
TOK(OpenBrace, "[")
TOK(CloseBrace, "]")

TOK(Invalid, "")
TOK(EndOfFile, "EOF")

#undef KEYWORD
#undef TOK
