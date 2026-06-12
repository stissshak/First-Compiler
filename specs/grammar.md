# MPL — Grammar

MPL is a C-like language. This document defines the lexical structure and the
full syntax in EBNF. Notation: `[x]` — optional, `{x}` — zero or more,
`|` — alternative, `"x"` — literal text.

## 1. Lexical structure

### Alphabet

Source is ASCII text. Whitespace (spaces, tabs, newlines) separates tokens and
is otherwise ignored.

### Comments

```
// line comment, to the end of line
/* block comment, no nesting */
```

Comments are removed before tokenization and have no semantic effect.

### Keywords

Reserved, cannot be used as identifiers:

```
int short long float char bool byte void
struct typedef const extern
if else while for return break continue
true false sizeof typeid
enum union switch case default     // reserved for future use
```

### Identifiers

```
identifier = letter , { letter | digit } ;
letter     = "A".."Z" | "a".."z" | "_" ;
digit      = "0".."9" ;
```

### Literals

```
int-literal    = digit , { digit } ;                       (* 64-bit value *)
float-literal  = { digit } , "." , digit , { digit } ;
char-literal   = "'" , character , "'" ;
string-literal = '"' , { character | escape } , '"' ;
bool-literal   = "true" | "false" ;
escape         = "\n" | "\t" | "\r" | "\0" | "\\" | '\"' ;
```

The type of an int literal is `int`, of a float literal `float`, of a string
literal `char*` (NUL-terminated, placed in read-only memory).

### Operators and delimiters

By precedence, weakest first (see §3):

```
=  +=  -=                                 assignment (right-assoc)
||                                        logical or
&&                                        logical and
|   ^   &                                 bitwise or, xor, and
==  !=                                    equality
<   >   <=  >=                            ordering
<<  >>                                    shifts
+   -                                     additive
*   /   %                                 multiplicative
```

Unary: `-  !  ~  &  *  (type)`. Postfix: `()  []  .  ->`.
Delimiters: `( ) { } [ ] , ; ...`

Reserved but not yet implemented: `*= /= %= &= |= ^= <<= >>=` (compile-time
error if used).

### Preprocessor

Before lexing, `#include "file"` is replaced by the contents of the file
(recursively). Diagnostics still point to the original file and line via a
source map. No other directives are supported.

## 2. Declarations

A program is a sequence of top-level declarations. Statements may appear only
inside function bodies. The entry point is `int main()` (see semantics.md).

```
program      = { declaration } ;
declaration  = struct-decl | typedef-decl | func-decl | var-decl ;

struct-decl  = "struct" , identifier , "{" , { field , ";" } , "}" ;
field        = type , identifier ;

typedef-decl = "typedef" , type , identifier , ";" ;

func-decl    = [ "extern" ] , type , identifier ,
               "(" , [ param-list ] , ")" , ( block | ";" ) ;
param-list   = "..." | param , { "," , param } , [ "," , "..." ] ;
param        = type , identifier ;

var-decl     = [ "const" ] , type , [ "const" ] , identifier ,
               [ "[" , int-literal , "]" ] , [ "=" , initializer ] , ";" ;
initializer  = expression
             | "{" , [ expression , { "," , expression } ] , "}" ;
```

Notes:
- a `func-decl` ending in `;` is a forward declaration and must be defined
  later in the file; an `extern` one must NOT have a body — it names an
  external (libc) function resolved at link time;
- `...` makes the function variadic and must be last in the parameter list;
- there is **no** `;` after the closing `}` of a struct;
- `const type` makes the variable constant; for pointers, leading `const`
  binds to the pointee (`const int* p`), `const` after the type binds to the
  variable itself (`int* const p`). See types.md;
- the `{...}` initializer form is allowed only for arrays and structs, and
  only in declarations (like C89). Nested `{}` are not supported.

## 3. Types

```
type       = base-type , [ "(" , [ type , { "," , type } ] , ")" ] , { "*" } ;
base-type  = "int" | "short" | "long" | "float" | "char" | "bool" | "byte"
           | "void" | identifier ;
```

`identifier` as a type names a struct or a typedef alias. The `(...)` suffix
forms a function type: `int(int, int)` is "function taking two ints, returning
int" — used to declare function pointers (`int(int, int) f = add;`). Stars
bind after the function suffix: `int(int)*` is a pointer to a function value.
Array-ness is part of the declarator (`int a[10]`), not of `type`.

## 4. Statements

```
statement   = block | if-stmt | while-stmt | for-stmt
            | "return" , [ expression ] , ";"
            | "break" , ";"
            | "continue" , ";"
            | ";"                                       (* null statement *)
            | var-decl
            | expression , ";" ;

block       = "{" , { statement } , "}" ;
if-stmt     = "if" , "(" , expression , ")" , statement ,
              [ "else" , statement ] ;
while-stmt  = "while" , "(" , expression , ")" , statement ;
for-stmt    = "for" , "(" , [ var-decl | expression ] , ";" ,
              [ expression ] , ";" , [ expression ] , ")" , statement ;
```

## 5. Expressions

Assignment is an expression; its value is the assigned value. Binary operators
parse by precedence climbing with the table from §1 (all left-associative
except assignment).

```
expression  = assignment ;
assignment  = binary , [ ( "=" | "+=" | "-=" ) , assignment ] ;
binary      = unary , { binop , unary } ;             (* precedence table *)
unary       = ( "-" | "!" | "~" | "&" | "*" ) , unary
            | "(" , type , ")" , unary                (* cast *)
            | postfix ;
postfix     = primary , { "(" , [ args ] , ")"        (* call *)
                        | "[" , expression , "]"      (* index *)
                        | "." , identifier            (* field *)
                        | "->" , identifier } ;       (* field via pointer *)
args        = expression , { "," , expression } ;
primary     = int-literal | float-literal | char-literal | string-literal
            | bool-literal | identifier
            | "(" , expression , ")"
            | "sizeof" , "(" , ( type | expression ) , ")"
            | "typeid" , "(" , expression , ")" ;
```
