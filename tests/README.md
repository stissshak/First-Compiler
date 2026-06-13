# Stress / extreme-case tests

Programs that push the compiler into corners: register pressure, the SysV
argument boundary, deep recursion, large frames, deep nesting, heavy
int↔float coercion, multi-level pointers, and full bitwise coverage.

Each test is **self-verifying**: it computes a value with a known answer and
prints `PASS` or `FAIL (...)`. Run the whole suite with:

```sh
bash tests/run.sh
```

A test counts as passing only if it compiles, links, runs, exits 0, and prints
no `FAIL` line. The runner covers both this directory and `points/`.

## `points/` — one test per graded task (points.md)

| File | Task | Covers |
|------|------|--------|
| `A_1_1_sized_types.mpl`           | A.1.1  | sizes of every type, two's-complement wrap, `uint` unsigned compare/divide/shift, `byte` |
| `A_1_6_nullable.mpl`              | A.1.6  | `null` init/compare/reset for several pointer types, list terminator |
| `A_1_8_implicit_conversions.mpl`  | A.1.8  | the cast matrix's implicit cases: int↔float, int↔char, int↔bool, integer family, `void*`↔`T*` |
| `A_1_13_metafunctions.mpl`        | A.1.13 | `sizeof` (types/array/non-evaluated expr) and `typeid` name strings |
| `A_2_18_pointers.mpl`             | A.2.18 | address-of, deref r/w, mutate-through-pointer, `**`, pointer arithmetic, `const T*` |
| `A_3_9_raw_pointers.mpl`          | A.3.9  | `void*` from `malloc`, `void*`↔`T*`, one block viewed as int then char |
| `A_3_16_ffi.mpl`                  | A.3.16 | libc via `extern`: `strcmp`, `atoi`, `toupper`, `strlen` (long), `sqrt` (double), `memcpy` |

## Stress tests

| File | What it stresses |
|------|------------------|
| `register_pressure_gp.mpl`  | 11 simultaneously-live int temporaries — exactly fills the GP register pool |
| `register_pressure_sse.mpl` | 16 live float temporaries — exactly fills the XMM pool |
| `many_args.mpl`             | >6 args (stack-passed), interleaved int/float args, varargs |
| `deep_recursion.mpl`        | fib / factorial / Ackermann / deep countdown — frame & return plumbing |
| `many_locals.mpl`           | 40 scalars + large arrays — frame size, alignment, far rbp offsets |
| `deep_nesting.mpl`          | 12-deep `if`, 4-deep loops, `break`/`continue` |
| `int_float_mix.mpl`         | int↔float coercion at init/assign/compound/operand/arg/return |
| `pointers_arrays.mpl`       | triple indirection, pointer arithmetic, struct copy, `.`/`->` chains |
| `bitwise_shifts.mpl`        | `& | ^ ~ << >>` + compound forms, hex/binary literals |

## Discovered limits (codegen has no register spilling)

The register allocator pops from a fixed pool and **does not spill**, so an
expression that needs more simultaneously-live temporaries than there are
registers aborts the *compiler* (`vector::back()` on an empty pool).

- **GP**: 11 registers (`rbx, r12–r15, rsi, rdi, r8–r11`). A right-associated
  chain of depth ≤ 11 compiles; depth 12 aborts.
- **SSE**: 16 registers (`xmm0–xmm15`). Depth ≤ 16 compiles; depth 17 aborts.

The pressure tests sit exactly at these ceilings, so they double as a guard:
if pool sizes change, they start failing.

## Bugs these tests found (now fixed)

- `>6` arguments emitted `[rbp16]` instead of `[rbp+16]` (`memOf` ignored positive offsets).
- Float **return** values were moved to `rax` instead of `xmm0`.
- Float **parameters** were read from GP arg registers instead of `xmm0–7`.
- Implicit conversion to `bool` did not normalize to 0/1 (`bool b = 5` stored 5).

## Known limitations (not bugs)

- **Struct fields can't be arrays** (`field = type identifier`); use a pointer
  field into a standalone array instead.
- No register spilling — see limits above.
- A libc function whose name is a **NASM reserved word** (`abs`, `seg`, `wrt`, …)
  can't be `extern`'d: the symbol must match libc exactly, but NASM rejects
  `extern abs`. Pick a non-reserved entry point (`labs`, a wrapper, etc.).
- `char` and `byte` are **signed**: `(int)(char)0xE8` is `-24`, not `232`.
