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

## Backend (Code generation)
Target: x86-64, NASM syntax (`nasm -f elf64`), SysV ABI.
- [ ] Functions
  - [x] prologue / epilogue (push rbp, mov rbp rsp, leave, ret)
  - [x] frame layout + 16-byte aligned `sub rsp`
  - [x] parameter passing (6 regs + stack args, spilled to frame)
  - [x] single epilogue label (`.Lreturn_<fn>`)
- [ ] Expressions
  - [ ] literals (int) + identifier loads
  - [ ] arithmetic (add, sub, imul)
  - [ ] div / mod (rax:rdx, cqo, idiv)
  - [ ] comparisons (cmp / setcc / movzx)
  - [ ] logical && / || (short-circuit)
  - [ ] assignment (=, +=) + initializer stores
  - [ ] function calls (arg setup, caller-saved spills, alignment fixup)
- [ ] Statements
  - [ ] return
  - [ ] if / while / for (branches + labels)
  - [ ] break / continue
- [ ] Register allocator
  - [x] free / in-use register pools (acquire / release)
  - [ ] spill to stack when pool exhausted
- [ ] Globals + data sections (.data / .rodata / .bss)
- [ ] Floating point (xmm registers)
- [ ] Structs / arrays / pointers in codegen

## IR & optimization (future)
- [ ] Intermediate representation
- [ ] -O0 / -O1 / -O2 passes