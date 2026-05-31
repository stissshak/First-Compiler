# MPL — a first compiler

![Status](https://img.shields.io/badge/status-in_progress-yellow)
![Stage](https://img.shields.io/badge/stage-backend_(codegen)-blue)
![License](https://img.shields.io/badge/license-MIT-green)

A from-scratch compiler for **MPL**, a small C-like language, written in C++23.
It is a learning project: every stage — preprocessing, lexing, parsing, semantic
analysis, and x86-64 code generation — is hand-written, with no parser
generators or backend libraries.

## Pipeline

Source flows through the stages below; each is a separate module under `inc/`
and `src/`:

```
.mpl ──▶ Preprocessor ──▶ Lexer ──▶ Parser ──▶ Analyser ──▶ CodeGenerator ──▶ .asm
         #include/#define  tokens    AST        types/scopes  x86-64 NASM
```

| Stage | Files | Status |
|-------|-------|--------|
| Preprocessor | `Preprocessor.{hpp,cpp}` | `#include "..."`, basic `#define` / `#ifdef` |
| Lexer | `Lexer.{hpp,cpp}`, `Token.hpp` | all operators, int/double literals |
| Parser | `Parser.{hpp,cpp}`, `Ast.hpp` | decls, statements, expressions |
| Analyser | `AstAnalyser.{hpp,cpp}`, `Types.hpp` | symbol table / scopes; type checking WIP |
| CodeGenerator | `CodeGenerator.{hpp,cpp}` | functions, frames, params — expressions WIP |
| Diagnostics | `Logger.hpp`, `SourceMap.hpp`, `Input.{hpp,cpp}` | source-mapped messages |

There is also an `AstPrinter` for dumping the parsed AST during development.

## Code generation

The backend targets **x86-64, NASM syntax** (`nasm -f elf64`), following the
SysV ABI (integer args in `rdi, rsi, rdx, rcx, r8, r9`, return in `rax`,
16-byte stack alignment at calls). Emission is a buffered single pass: each
function's body is accumulated into a string while the frame size is
discovered, then prologue + body + epilogue are composed in order.

What currently emits: function labels (`global`), prologue/epilogue
(`push rbp` / `mov rbp, rsp` / aligned `sub rsp` / `leave; ret`), incoming
parameter spills to the stack frame, local-variable frame layout, and lexical
shadowing. Expression evaluation, calls, and control flow are next — see the
roadmap.

## Build

Requires `g++` with C++23 support.

```sh
make          # debug build  -> bin/comp
make release  # optimized build
make clean
```

## Usage

```sh
bin/comp <input-file> <output.asm>
```

Then assemble and link the emitted NASM:

```sh
nasm -f elf64 output.asm -o output.o
ld output.o -o program          # or: gcc output.o -o program
```

## Status & roadmap

This is an active work in progress; the frontend is largely functional and the
backend is under construction. See [`roadmap.md`](roadmap.md) for the detailed,
checkbox-level state of every stage.

## License

MIT.
