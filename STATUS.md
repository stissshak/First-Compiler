# MPL — Project Status

> Companion to `project v1.0.md` (the ТЗ). Tracks (1) base-compiler completion and
> (2) the допы roadmap with scores. Last audited: 2026-06-06.

---

## Part 1 — Base compiler: is it done?

**Short answer: not yet, but the backend is now real.** The frontend (lexer + parser)
is mostly complete, semantic analysis is partial but now annotates the AST with types,
and the **code generator went from stubs to a working core** — control flow, arithmetic,
calls, pointers and most expressions now emit code. A handful of visitors remain stubbed.

### Per-phase status

| Phase | State | Notes |
|---|---|---|
| **Lexer** | ✅ ~92% | keywords (now incl. `bool`/`true`/`false`), idents, int/float/char/string literals, line + block comments, all operators; emits `Eof` |
| **Parser** (recursive descent) | 🟡 ~82% | decls, exprs, if/else/while/for, break/continue/return, casts, index/field access, `bool`/`true`/`false` all parse |
| **Semantic** (AstAnalyser) | 🟡 ~55% | scopes, dup-decl, type checking + return/loop checks, **`main` presence + `int`-return validation**, **every `Expr` now carries `resultType`** (typed AST feeds codegen); some checks still missing |
| **CodeGen** (x86-64, **NASM** syntax) | 🟡 ~60% | reg-alloc pool (int+SSE), prologue/epilogue, System V param spill, sections (.data/.rodata/.bss/.text); emits: blocks+scoping, if/while/for, break/continue/return, local var init, assign + compound `+=`/`-=`, short-circuit `&&`/`\|\|`, int & float arithmetic/compare, pointer arith, `idiv`, `&`/`*`, indexing, calls (caller-side spill, variadic `al=0`), Int/Bool/String literals, identifiers. **Still stubbed:** `CastExpr`, `AccessExpr` (struct field), `FloatLiteral`, `CharLiteral`, struct sizing/decl, builtins |

### Mandatory requirements — gap list

Marks: ✅ done · 🟡 partial · ❌ missing/stub

**Types** — ✅ int, float, char, void, **bool** (`true`/`false`, in cast matrix + parser + codegen), array, struct · 🟡 string (modeled as `char*`, OK as design choice) · ❌ **uint**, ❌ **type aliases** (`Typedef` token exists, unparsed)

**Expressions** — ✅ unary -, !, arithmetic, logical, comparison, cast, indexing, field access, scalar literals · ❌ namespace access, ❌ array literal, ❌ struct literal *(literals needed for init)*

**Control flow** — ✅ blocks, if/else, while, for, break, continue, return(value+void) · ❌ null statement (`;`)

**Declarations** — ✅ variables, functions, structs · ✅ `main` (presence + `int`-return now validated in AstAnalyser) · ❌ **mutability mechanism** (no let/var/const), ❌ type aliases, ❌ namespaces

**Name resolution** — ✅ lexical scoping, shadowing, dup-decl error · 🟡 use-before-decl (error path inconsistent), 🟡 type checking (incomplete: e.g. condition-is-bool not enforced), 🟡 control-flow (no "all paths return")

**Builtins** — ❌ **print, input, exit, panic, assert** (none implemented)

**Runtime errors** — ❌ division-by-zero, ❌ array-out-of-bounds (need codegen first)

**CLI** — 🟡 positional args only (no `-o` parsing), 🟡 `--dump-ast` always runs (no flag), ❌ `--dump-tokens`, ❌ exit codes

**Diagnostics** — ❌ `<file>:<line>:<col>: error:` format (Logger prints `[ERROR]` only), ❌ uses `throw std::runtime_error` for syntax errors instead of `std::expected`/`std::optional` (spec violation) · ✅ no global error state

**Build** — ✅ Makefile, C++23, targets build/run/debug/clean · ⚠️ no auto assemble+link (manual `as`/`ld`)

### Critical path to "base complete"

1. **Finish remaining codegen visitors** — `CastExpr`, `AccessExpr` (struct field load/store),
   `FloatLiteral`, `CharLiteral`, plus struct sizing/layout (`sizeOf` has a `Custom` TODO).
   *(the bulk — control flow, arithmetic, calls, pointers — now emits; this is the tail)*
2. **Builtins** — print / input / exit / panic / assert.
3. **uint** type. *(bool + `true`/`false` ✅ done)*
4. **Runtime checks** — div-by-zero (codegen guard is currently commented out), array bounds
   → `runtime error: ... at line N`, nonzero exit.
5. **Error handling** — migrate parser to `std::expected`/`optional`; diagnostics format with file:line:col.
6. **Mutability mechanism** (let/var or const), **type aliases**, **null statement**.
7. **CLI/driver** — `-o`, `--dump-tokens`, `--dump-ast` flags, exit codes, auto assemble+link.
8. ~~**`main` validation**~~ ✅ done.

> Lower priority / optional-per-spec: namespaces, array & struct literals (literals "на
> усмотрение студента" for syntax, but some init form is required).

---

## Part 2 — Допы (extra tasks): what we can do

### Scoring rules

| Item | Points |
|---|---|
| Easy (L1) | 3 each — **max 5 count** (cap = 15) |
| Medium (L2) | 10 |
| Hard (L3) | 20 |
| **Hard that «Расширяет» a Medium** | **10 + 20 + 5 = 35** (chain bonus +5) |

Chain bonus applies **only** when the spec marks the hard as *«Расширяет»* the medium.
A hard that merely *«Требует»* something is a standalone 20 (no chain).

### Status of every доп

Marks: ✅ done · 🔨 in progress · 🎯 committed roadmap · 💡 cheap candidate · 🔒 blocked (needs an unbuilt medium first)

#### Level 1 — easy (cap 5; we already have 4)

| ID | Feature | Status |
|---|---|---|
| A.1.1 | Sized numeric types + suffixes | ✅ done |
| A.1.5 | Nullable types | ✅ done |
| A.1.6 | Type inference | ✅ done |
| A.1.7 | Implicit cast | ✅ done |
| A.1.12 | **Block comments + nesting** | 🔨 last easy slot → fills cap (≈1–2h; basic works, only nesting depth-counter left) |
| A.1.8 / A.1.11 / others | bitwise / sizeof-typeof / … | ⛔ won't count — easy cap already reached at 5 |

#### Level 2 — medium

| ID | Feature | Status |
|---|---|---|
| A.2.14 | Pointers | 🎯 stepping stone to A.3.7 (counts inside the chain) |
| A.2.3 | Struct methods | 🎯 committed (10) — gateway to A.2.4/2.5/2.6/2.11 |
| A.2.9 | Function overloading | 💡 optional; unlocks cheap A.3.1 chain |

#### Level 3 — hard

| ID | Feature | Status |
|---|---|---|
| A.3.7 | Raw/void/function pointers, byte, const-ptr — *Расширяет A.2.14* | 🎯 chain with A.2.14 = **35** |
| A.3.12 | FFI / C interop — *Требует* A.2.14/A.3.7 | 🎯 standalone **20** (building) |
| A.3.1 | Overload + implicit cast — *Расширяет A.2.9* | 💡 cheap chain (35) if A.2.9 done |
| A.3.2/3/4/5/6/8/9/10/11 | closures, generics, modules, variadics, macros, generators, stdlib | 🔒 each needs an unbuilt medium first |
| B.3.1 | Register allocation | 🟡 reg pool live (int+SSE alloc/free, caller-side spill); core codegen now emits, naive allocator — real allocation pass still TODO |
| B.3.2 | Alt parser method | 🔒 parser rewrite |

### Score tally

| Source | Points |
|---|---|
| Banked: 4 easy (A.1.1/1.5/1.6/1.7) | 12 |
| A.1.12 block comments (last easy) | +3 → easy cap 15 |
| Pointer chain A.2.14 → A.3.7 | +35 |
| FFI A.3.12 (standalone) | +20 |
| Struct methods A.2.3 | +10 |
| **Committed total** | **80** |

Further upside: each future *«Расширяет»* pair (e.g. A.3.1←A.2.9, A.3.10←A.2.6) is worth **35**.
