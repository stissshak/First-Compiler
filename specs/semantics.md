# MPL — Semantics

## 1. Program structure and entry point

A program is a set of top-level declarations; executable code lives only in
function bodies. The program must define `int main()` — its return value is
the process exit code. Missing `main`, or `main` with a different return
type, is a compile error.

Declarations are processed top to bottom: a name must be declared before its
first use (like C). A function declared without a body (`int f(int);`) is a
forward declaration and **must** be defined later in the file — otherwise it
is a compile error. External functions are declared with `extern` (see §8);
only those may stay without a definition. Defining an `extern` function is an
error.

## 2. Evaluation strategy

- **Call by value.** Every argument is copied into the callee. Pointers copy
  the address, so callees can mutate through pointers (`scanf("%d", &x)`).
- **Value semantics of assignment.** `a = b` copies the value; for structs a
  byte-wise copy of the whole record. Assignment is an expression whose value
  is the assigned value; it associates right (`a = b = c` is not supported as
  a chain — `b = c` yields a value, but chained writes are untested; use
  separate statements).
- **Evaluation order** of binary operands is left to right. `&&` and `||`
  short-circuit: the right operand is evaluated only if needed; the result is
  `bool` (0 or 1).

## 3. Variables, scope, lifetime

- Variables are declared with a type and live from the declaration to the end
  of the enclosing block (stack allocation). Inner declarations shadow outer
  ones; redeclaring a name in the same scope is a compile error.
- **Use of an uninitialized variable is a compile error.** A variable counts
  as initialized when it: has an initializer; is assigned with `=`; has its
  address taken with `&` (so `scanf("%d", &n)` initializes `n`). Arrays and
  structs count as initialized at declaration, since they are filled
  element-by-element / field-by-field.
- `const` variables must be initialized at declaration and cannot be assigned
  (including `+=`/`-=`). Stores through a `const T*` pointer are compile
  errors. Function names are not assignable.
- Global variables are allowed: with a literal initializer they go to static
  data; without one they are zero-initialized. Non-literal global
  initializers are not supported.

## 4. Control flow

`if`/`else`, `while`, C-style `for`, `break` / `continue` (innermost loop
only, error outside a loop), `return` (with a value in non-`void` functions,
bare in `void` ones), blocks introducing scope, and the null statement `;`
have the usual C meaning. Conditions are any arithmetic/pointer/bool value;
non-zero is true.

## 5. Functions

- Fixed arity, except variadic declarations ending in `...` — the extra
  arguments are type-checked only against the fixed prefix. Variadic
  functions can be declared (for libc: `printf`, `scanf`) but not defined in
  MPL.
- Recursion is allowed.
- **Overloading** — several functions may share a name if their parameter
  signatures differ. A call resolves to the overload whose parameters match the
  argument types **exactly** (no implicit conversions are used to choose between
  candidates); zero matches or more than one is a compile error. When a name has
  a single definition the call behaves like an ordinary call (arguments convert
  normally). Redefining the same signature is an error. Symbols are name-mangled
  by parameter types so overloads link distinctly; `main` and `extern` functions
  keep their exact name for C linkage.
- A function name used as a value denotes the function's address and has the
  function type (`int(int, int) f = add;`). Calls through such variables are
  indirect calls; the signature must match exactly. Taking the address of an
  *overloaded* name is an error (no context to pick an overload).
- Structs may be passed and returned **through pointers only**; passing a
  struct by value is not implemented (known limitation).

## 6. Side effects

Functions may freely perform I/O (via the C library) and mutate variables
through pointers. There is no purity tracking.

## 7. Memory model

- Locals — stack, freed automatically at block exit.
- Globals, string literals — static memory.
- Dynamic memory — `malloc` / `free` from libc (declared by the program, §8).
  No garbage collection; lifetime is managed manually by the programmer.

## 8. Built-in functions and foreign functions (libc)

### Built-ins

The following functions are pre-declared by the compiler and usable without
any declaration. They are backed by libc:

| Builtin  | Signature          | Backed by | Notes                              |
|----------|--------------------|-----------|------------------------------------|
| `print`  | `int(char*, ...)`  | `printf`  | formatted output                   |
| `input`  | `int(char*, ...)`  | `scanf`   | formatted input, pass addresses    |
| `exit`   | `void(int)`        | `exit`    | terminate with the given code      |
| `panic`  | `void(char*)`      | `puts`+`exit(1)` | print message, exit 1       |
| `assert` | `void(bool)`       | inline check | on false: `runtime error: assertion failed at line <N>`, exit 1 |
| `malloc` | `void*(int)`       | `malloc`  | allocate raw memory                |
| `free`   | `void(void*)`      | `free`    | release it                         |

`assert` compiles to an inline test, not a call — the condition's source line
is reported like the other runtime checks. Defining a function with a builtin
name is a compile error.

### extern

Everything else from the C world is declared with `extern` and called with
the System V AMD64 ABI, resolved by the linker:

```
extern float sqrt(float x);
extern int puts(char *s);
```

At the boundary: scalars and pointers only; MPL strings are already
NUL-terminated `char*`. Struct-by-value does not cross the boundary.

## 9. Compile-time diagnostics

Errors and warnings go to stderr in the format

```
<file>:<line>:<col>: error: <message>
<file>:<line>:<col>: warning: <message>
```

with positions tracked through `#include`. Any error stops compilation before
code generation and the compiler exits with a non-zero code. Syntax errors
stop at the first one; semantic analysis reports all it finds.

## 10. Runtime errors

The generated code checks, at runtime:

- integer division / remainder by zero;
- array indexing out of bounds (`a[i]` where `a` is a `T[N]`, checked as
  unsigned so negative indices are caught too);
- dereference of a null pointer (`*p`, `p->f`, `p[i]` for pointer `p`).

On violation the process prints

```
runtime error: <message> at line <N>
```

to stderr and exits with code 1. Indexing through a *pointer* is bounds-
unchecked (the size is unknown) — only the null check applies.
