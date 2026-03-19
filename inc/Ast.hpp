// MPL/inc/Ast.hpp

#pragma once

#include <string_view>
#include <string>

struct Node{
    virtual ~Node() = default;
};

struct Expr : Node {};
struct Stmt : Node {};
struct Decl : Node {};
struct Type : Node {};

// Expr

struct IntLiteral : Expr{
    int value;
};

struct FloatLiteral : Expr{
    double value;
};

struct StringLiteral : Expr{
    std::string value;
};

struct Identifier : Expr{
    std::string name;
};

// Operations

enum class BiraryOp{
    Add, Sub, Mul
};
