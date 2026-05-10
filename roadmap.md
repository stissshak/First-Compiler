# Roadmap for my compiler
![Status](https://img.shields.io/badge/status-in_progress-yellow)
![Stage](https://img.shields.io/badge/stage-frontend-blue)
![Progress](https://img.shields.io/badge/progress-34%25-orange)
![License](https://img.shields.io/badge/license-MIT-green)
## CLI

- [ ] Input/output
  - [ ] Name for input file(s)
  - [ ] Name for output file (-o)
  - [ ] Stop after stage: -E, -S, -c
- [ ] Optimization levels
  - [ ] -O0
  - [ ] -O1
  - [ ] -O2
- [ ] Diagnostics
  - [ ] -Wall
  - [ ] -Werror

## Source (Preprocessor)
- [ ] File inclusion
  - [x] #include "..."
  - [ ] #include <...>
  - [ ] include guard tracking
- [ ] Macros
  - [x] #define object-like (WIP)
  - [ ] #define function-like
  - [ ] Variadic macros
  - [ ] # and ##
- [ ] Conditional compilation
  - [ ] #if / #elif / #else / #endif
  - [x] #ifdef / #ifndef (WIP)

## Frontend
### Lexer
- [x] Operators
  - [x] Arithmetic
  - [x] Logic
  - [x] Bit
  - [x] Comparison
  - [x] Compound assignment (+=, -=, ...)
- [ ] Literals
  - [x] int decimal
  - [ ] int hex (0x)
  - [ ] int octal (0...)
  - [x] double
  - [ ] char escapes (\n, \xNN)
  - [ ] string concatenation

### Parser
- [ ] Declarations
  - [x] VarDecl
  - [x] FuncDecl
  - [x] StructDecl
  - [ ] TypedefDecl
- [ ] Statements
  - [x] ExprStmt
  - [x] IfStmt
  - [x] WhileStmt
  - [x] ForStmt
  - [x] SwitchStmt
  - [x] GotoStmt

### Analyser
- [x] Symbol table / scopes
- [ ] Type checking
  - [ ] Implicit conversions
  - [ ] lvalue/rvalue
- [ ] Diagnostics with source locations