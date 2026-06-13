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

Requirements: CMake (≥ 3.20), a C++23 compiler, `nasm`, `gcc` (linker).

```sh
cmake -B build && cmake --build build      # compile -> bin/comp
bin/comp examples/structs.mpl -o structs   # compile + assemble + link
./structs

# stop at an earlier stage (gcc-style):
bin/comp prog.mpl -S        # assembly only  → prog.asm
bin/comp prog.mpl -c        # object only    → prog.o
bin/comp prog.mpl -E        # preprocess to stdout
```

The driver mirrors `gcc`: with no `-S`/`-c`/`-E` it compiles, assembles
(`nasm -felf64`) and links (`gcc -no-pie … -lm`) a runnable executable,
defaulting to `a.out`; intermediates go to the temp dir and are auto-removed.
CMake options: `-DCMAKE_BUILD_TYPE=Release`, `-DENABLE_SANITIZERS=ON`; targets
`ctest` (test suite) and `--target run`. Full CLI: `comp [options] <file.mpl>`
with `-o`, `-S`, `-c`, `-E`, `--dump-tokens`, `--dump-ast`, `-h`.

## Implemented extras (допы)

Codes follow `points.md` (v2.0).

- **A.1.1** sized numeric types — `char`/`bool`/`byte` = 1, `short` = 2,
  `int`/`uint` = 4, `long`/`float` = 8; two's-complement wrap, and `uint`
  unsigned compare/divide/right-shift
- **A.1.6** nullable — `null` literal, null pointers, runtime error on null
  dereference
- **A.1.8** implicit conversions via the cast matrix — including int↔float
  (inserted `cvtsi2sd`/`cvttsd2si` at init/assign/operands/args/return) and
  normalization to `bool` (0/1)
- **A.1.9** extended operators — bitwise `& | ^ ~`, shifts `<< >>`, all
  compound assignments (`+= … >>=`), `++`/`--`. Shifts run on 64-bit
  registers, so the count is taken mod 64; right shift is arithmetic for
  signed, logical for `uint`
- **A.1.13** metafunctions — compile-time `sizeof(type|expr)` (operand not
  evaluated), `typeid(expr)`
- **A.2.8** function overloading — one name, several parameter signatures;
  resolution by exact parameter match (no implicit conversions used to choose
  between candidates). Symbols are type-mangled (`add` → `add_ii`); `main` and
  `extern` libc names keep their exact name
- **A.2.18** pointers — `&`/`*`, pointer arithmetic with element scaling,
  multi-level pointers, pointer casts, `const int*` vs `int* const`
- **A.3.9** (extends A.2.18) raw/generic pointers — `void*` ↔ any pointer,
  `malloc`/`free`, non-arithmetic `byte`, function pointers (declare, pass,
  reassign, indirect call)
- **A.3.16** FFI / C interop — `extern float sqrt(float);` declares a C-ABI
  function resolved at link time; full SysV caller side including varargs
  (`al` = xmm count, stack args beyond 6, float args/returns). Non-extern
  declarations must be defined — a missing body is a compile error, not a
  linker error
- **floats** — IEEE-754 `double`; `inf`/`nan` literals (`-inf` negated) and
  IEEE-correct comparisons (NaN is unordered, so `nan != nan`)
- **base built-ins** — `print`/`input`/`exit`/`panic`/`assert` (plus
  `malloc`/`free`) are pre-declared and libc-backed; `assert` compiles to an
  inline runtime check with the source line in the message

## Testing

`tests/` holds self-verifying programs run by `tests/run.sh` (compile → link →
run; a test passes only on exit 0 with no `FAIL` line). `tests/points/` has one
program per graded task (A.1.1 … A.3.16); the rest are stress tests (register
pressure, >6 args, deep recursion/nesting, coercion, pointers, bitwise, float
specials). The suite also pins compiler limits: a right-nested expression of
depth 12 (GP) / 17 (SSE) exhausts the register pool — see below.

## Known limitations

- structs cannot be passed to or returned from functions by value (pointers
  work); struct **fields cannot be arrays** (`field = type identifier`);
  struct literals don't nest (`{{...}}`)
- **no register spilling**: the allocator has 11 GP + 16 SSE registers, so an
  expression needing more simultaneously-live temporaries aborts the compiler
- a non-zero **integer literal initializing a float global** is not converted
  (`float g = 5;` is wrong — write `5.0`); `-inf` as a global has the same
  non-literal-init limitation
- an `extern` whose name is a **NASM reserved word** (`abs`, `seg`, `wrt`, …)
  can't be linked, since the symbol must match libc exactly
- namespaces and `switch` are reserved but not implemented
- code organization: compiler phases are separate modules/classes but not
  wrapped in namespaces yet; the parser reports errors by exception rather
  than `std::expected`
