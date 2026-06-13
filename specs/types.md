# MPL — Type System

## 1. Built-in types

| Type    | Size (bytes) | Description                                      |
|---------|--------------|--------------------------------------------------|
| `int`   | 4            | signed integer, two's complement                 |
| `uint`  | 4            | unsigned integer                                 |
| `short` | 2            | signed integer                                   |
| `long`  | 8            | signed integer                                   |
| `float` | 8            | IEEE-754 double precision (C's `double`)         |
| `char`  | 1            | character; arithmetic, like in C                 |
| `bool`  | 1            | `true` / `false`, stored as 1 / 0                |
| `byte`  | 1            | raw data, **non-arithmetic** (see §5)            |
| `void`  | —            | absence of a value; only as return type / `void*`|

Agreed deviations from the base table: `char` is arithmetic and there is a
single float width (8 bytes). `char` and `byte` are **signed**. Signed integers
are two's complement; overflow wraps around. `uint` loads zero-extend and use
unsigned comparison, division and right shift; mixed `int`/`uint` comparisons
compare unsigned.

Bitwise operators (`& | ^ ~ << >>`), their compound-assignment forms, and
`++`/`--` apply to the integer family. Shifts execute on a 64-bit register, so
the shift count is taken **modulo 64** (matching x86 `SAL`/`SAR`/`SHR`); a
negative count uses its unsigned bit pattern (`x >> -1` is `x >> 63`). Right
shift is arithmetic for signed integers and logical for `uint`.

`float` follows IEEE-754: the literals `inf` and `nan` denote positive infinity
and NaN (`-inf` is `inf` negated), and division does **not** trap (`1.0/0.0` is
`inf`, `0.0/0.0` is `nan`). NaN is unordered: `<`, `>`, `<=`, `>=` and `==`
against a NaN are all false, and `!=` is true (so `nan != nan`).

## 2. Derived types

| Type          | Size | Notes                                              |
|---------------|------|----------------------------------------------------|
| `T*`          | 8    | pointer; `void*` converts to/from any pointer      |
| `T[N]`        | N·sizeof(T) | fixed array; size is a literal              |
| `R(P1, ...)`  | 8    | function type; variables of it hold a function address |
| `struct S`    | sum of fields + padding | nominal record type            |

Struct layout follows C: each field is aligned to its own size, the struct is
padded to the alignment of its largest field (see codegen.md §5).

## 3. General rules

- **Nominal typing** — two struct types are compatible only if they have the
  same name. Two function types are compatible only if return type, parameter
  types and variadic-ness match exactly.
- **Strict static typing** — every expression has a type known at compile
  time; conversions follow the cast matrix below.
- **Value semantics** — assignment and argument passing copy the value. This
  includes whole structs (byte-wise copy). Pointers copy the address; the
  pointee is shared. Arrays are not assignable; in value position an array
  decays to a pointer to its first element, like in C.
- **Mutability is a property of the variable, not the type** — `const`
  variables must be initialized and cannot be assigned. For pointers,
  constness of the pointee is part of the pointer type (§6).
- **Type of literals** — int literal: `int` (the value itself is read as a
  64-bit number), float literal: `float`, char literal: `char`, string
  literal: `char*`, `true`/`false`: `bool`. No literal suffixes.

## 4. Conversions

Conversions are checked by a single cast matrix. Three outcomes:
*implicit* (allowed silently), *warn* (allowed with a compile warning),
*no* (compile error). The matrix is consulted for initialization, assignment,
argument passing and explicit casts.

| from \ to | int   | float | char  | void | struct | ptr  | bool  | byte |
|-----------|-------|-------|-------|------|--------|------|-------|------|
| **int**   | =     | impl  | impl  | no   | no     | warn | impl  | no   |
| **float** | impl  | =     | impl  | no   | no     | no   | no    | no   |
| **char**  | impl  | impl  | =     | no   | no     | warn | impl  | no   |
| **void**  | no    | no    | no    | =    | no     | no   | no    | no   |
| **struct**| no    | no    | no    | no   | same name | no | no    | no   |
| **ptr**   | warn  | no    | warn  | no   | no     | see below | no | no |
| **bool**  | impl  | no    | impl  | no   | no     | no   | =     | no   |
| **byte**  | no    | no    | no    | no   | no     | no   | no    | =    |

`short`, `long` and `uint` use the `int` row/column: conversions inside the
integer family are implicit; only storage size and signedness differ
(widening and narrowing both allowed; narrowing truncates, see codegen.md §3).

Pointer-to-pointer: implicit if either side is `void*`; otherwise the pointee
types are compared recursively. Assigning a `const T*` into a plain `T*`
(losing constness) produces a warning.

Numeric `int`/`char` ↔ pointer conversions produce a warning (needed for
`(int*)0` etc.).

Float → int truncates toward zero (`cvttsd2si`); int → float is exact for
values up to 2^53. Converting any scalar to `bool` normalizes to 0/1 (nonzero
becomes `1`), so `bool b = 5;` stores `1`.

## 5. byte

`byte` is deliberately crippled — it represents raw memory, not numbers:

- allowed: assignment, `==`, `!=`, taking the address, `byte*` arithmetic-free
  indexing through pointers;
- forbidden: all arithmetic, ordering comparisons, bitwise and logical
  operators, increment/decrement (compile errors);
- conversions: **explicit cast only**, and only to/from the integer family
  (`int`, `short`, `long`, `char`, `bool`). `(float)b` is an error. No
  implicit conversion in either direction.

## 6. const

```
const int x = 5;        // constant variable: must be initialized, no assigns
int* const p = &x;      // constant pointer variable: p cannot be reassigned
const int* q = &x;      // pointer to const: *q = ... is an error
```

- leading `const` on a pointer type binds to the deepest base type
  (`const int** r` is a pointer to pointer-to-const-int);
- `&x` where `x` is a `const` variable yields `const T*`;
- losing constness in a conversion warns (see matrix).

## 7. Metafunctions

- `sizeof(type)` / `sizeof(expr)` — size in bytes as an `int`, folded at
  compile time; the expression is **not** evaluated. `sizeof` of an array is
  the whole array (no decay).
- `typeid(expr)` — the type name as a `char*` string (`"int"`, `"Pt*"`,
  `"int(int, int)"`, ...), folded at compile time.
