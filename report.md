# MPL Compiler — Report

MPL is a small C-like language; the compiler is written from scratch in C++23
and targets x86-64 Linux through NASM assembly. No parser generators, no
LLVM — every stage is hand-written.

## Architecture

```
source ─▶ Preprocessor ─▶ Lexer ─▶ Parser ─▶ AstAnalyser ─▶ CodeGenerator ─▶ .asm
          #include +       tokens   AST       types, scopes,  NASM x86-64
          SourceMap                 (visitor)  init tracking
```

| Stage | Files | Job |
|-------|-------|-----|
| Preprocessor | `Preprocessor.hpp`, `Input.*` | splices `#include "..."`, builds a **SourceMap** so every later diagnostic points to the original file:line:col |
| Lexer | `Lexer.*`, `Token.hpp` | hand-written scanner; keywords, literals, multi-char operators (3→2→1 longest match), comments |
| Parser | `Parser.*`, `Ast.hpp` | recursive descent; precedence climbing for expressions; builds an owning AST (`unique_ptr`), every node carries a source offset |
| Analyser | `AstAnalyser.*`, `Types.hpp` | visitor over the AST: scoped symbol table, type checking via a single cast matrix, initialization tracking, const enforcement; counts errors and blocks codegen |
| CodeGenerator | `CodeGenerator.*` | visitor emitting NASM; register pool, stack frames, SysV calls, runtime checks |

The AST is shared by three independent visitors (printer, analyser,
generator) through a classic `accept`/`visit` interface.

## Key design decisions

- **Recursive descent + precedence climbing** — simplest approach that
  handles a C-like grammar; error messages stay precise because every parse
  function knows what it expected.
- **Native x86-64, no IR** — one pass from typed AST to NASM keeps the whole
  backend readable (~1000 lines). Optimizations were traded for simplicity.
- **Widen-on-load size model** — values in registers are always 64-bit;
  operand sizes (1/2/4/8) exist only in loads (`movsx`) and stores
  (sub-register writes). One rule covers `char`/`short`/`int`/`long`/`bool`/
  `byte` everywhere, including narrowing casts.
- **`int` is 4 bytes** — matches C's `%d` in `printf`/`scanf`, which makes
  the libc FFI work without surprises.
- **Single cast matrix** — all conversion legality (implicit / warn / error)
  is one table in `Types.hpp` consulted by every check; special rules
  (pointers, function types, `byte`, const) are small wrappers around it.
- **Runtime checks via raw syscalls** — `rt_error` uses `sys_write`/`sys_exit`
  directly, so checked programs don't depend on libc.
- **Function pointers without `(*)`** — the type is spelled
  `int(int, int) f = add;`; a function name in value position is its address.
  Same expressiveness as C, far simpler grammar.

## Build and run

Requirements: `g++` (C++23), `make`, `nasm`, `gcc` (linker).

```sh
make build                       # build the compiler into bin/comp
bin/comp examples/structs.mpl -e # compile + assemble + link → examples/structs.exe
./examples/structs.exe

# step by step:
bin/comp prog.mpl -o prog.asm
nasm -felf64 prog.asm -o prog.o
gcc -no-pie prog.o -o prog
```

Other make targets: `debug` (default, `-g`), `release` (`-O3`), `run`
(compiles and runs an example), `clean`. Compiler CLI:
`comp <file> [-o out] [-e] [--dump-tokens] [--dump-ast]`.

## Implemented extras (допы)

- **A.1.1** sized numeric types — `char`/`short`/`int`/`long` = 1/2/4/8 bytes
- **A.1.5** nullable — null pointers + runtime error on null dereference
- **A.1.6** type inference (initializer-based)
- **A.1.7** implicit conversions via the cast matrix
- **A.1.11** metafunctions — compile-time `sizeof(type|expr)`, `typeid(expr)`
- **A.2.14** pointers — `&`/`*`, pointer arithmetic with element scaling,
  pointer casts, `malloc`/`free`, runtime null-deref error
- **A.3.7** (extends A.2.14) — `void*` ↔ any pointer, non-arithmetic `byte`,
  function pointers (declare, pass, reassign, indirect call),
  `const int*` vs `int* const`
- **A.3.12** FFI / C interop — `extern float sqrt(float);` declares a C-ABI
  function resolved at link time; full SysV caller side including varargs
  (`al` = xmm count, stack args beyond 6, float args/returns). Non-extern
  declarations must be defined — a missing body is a compile error, not a
  linker error
- **base built-ins** — `print`/`input`/`exit`/`panic`/`assert` (plus
  `malloc`/`free`) are pre-declared and libc-backed; `assert` compiles to an
  inline runtime check with the source line in the message

## Known limitations

- structs cannot be passed to or returned from functions by value (pointers
  work); struct literals don't nest (`{{...}}`)
- compound assigns other than `+=` `-=` are parsed but rejected ("not
  supported yet"); `++`/`--` are not implemented
- no `uint`, no hex/binary literals, no literal suffixes; float has a single
  width and no exponent syntax
- preprocessor supports only `#include "..."`; no include guards (a file
  included twice is an error at semantic analysis)
- namespaces and `switch` are reserved but not implemented
- one semantic-analysis quirk: `sizeof(x)` of an uninitialized variable
  reports "not inited" although the operand is never evaluated
- code organization: compiler phases are separate modules/classes but not
  wrapped in namespaces yet; the parser reports errors by exception rather
  than `std::expected`
