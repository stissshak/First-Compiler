# MPL — Code Generation

## 1. Approach

Native **x86-64** assembly in NASM syntax, generated directly from the typed
AST in one pass — no intermediate representation and no optimizations. The
output `.asm` is assembled by `nasm -felf64` and linked against libc with
`gcc -no-pie` (the compiler's `-e` flag runs both steps).

Output file layout: `.data` (initialized globals), `.rodata` (string/float
literals, runtime-error messages), `.bss` (zero globals), `.text` (functions
and the runtime-error stub).

## 2. Registers and expression evaluation

Expressions are evaluated with a small register pool, LIFO:

- general purpose: `rbx r12-r15 rsi rdi r8-r11`
- floating point: `xmm0-xmm15`

`rax`/`rdx` stay out of the pool (used by `idiv`, returns), `rcx` is reserved
as the shift-count scratch (`cl`), `rsp`/`rbp` hold the frame. Each
expression node takes a register, computes into it, and frees its operands;
statements free the final register, so the pool never leaks across
statements.

## 3. Sizes: widen on load

The central size rule: **in registers every integer value is 64-bit; size
exists only at the memory boundary.**

- load: `movsx r64, byte/word/dword [mem]` (sign-extension; `mov r64` for
  8-byte types); floats: `movsd xmm, [mem]`;
- store: `mov [mem], r8/r16/r32/r64` — the sub-register write truncates for
  free, which implements narrowing conversions;
- all arithmetic, comparisons and conditions operate on full 64-bit
  registers and need no size handling at all.

Explicit narrowing casts re-extend in place (`movsx r64, r32`), bool casts
materialize 0/1 via `cmp`/`setne`. Int↔float casts use `cvtsi2sd` /
`cvttsd2si`.

## 4. Functions and calls (System V AMD64)

Frame: `push rbp; mov rbp, rsp; sub rsp, frameSize` (16-aligned), all locals
and spilled parameters live at fixed `rbp`-offsets; uniform 8-byte slots for
parameters. A single exit label (`leave; ret`).

Calls follow SysV:

- first 6 integer/pointer args in `rdi rsi rdx rcx r8 r9`, first 8 float
  args in `xmm0-xmm7`, the rest pushed on the stack right-to-left;
- `al` = number of xmm registers used (required by variadic callees);
- return value in `rax` (integers/pointers) or `xmm0` (floats);
- `rsp` is 16-byte aligned at the `call`; the generator tracks every push
  past the frame (`pushDepth`) so nested calls stay aligned.

Implementation: all argument values (and the callee address, for indirect
calls through function-pointer variables) are first evaluated into temporary
stack slots, then moved into their registers — this survives nested calls
and clobber-free loads. Live pool registers (both GP and xmm) are
caller-saved around the call.

Direct calls emit `call name`; calls through a function-pointer variable load
the address into `r10` and emit `call r10`. Extern declarations emit
`extern name` for the linker.

## 5. Data layout

- Struct fields are laid out in declaration order; each field aligned to its
  own alignment, total size padded to the largest field alignment (C rules).
  Field access is base address + constant offset; `s.f` costs one `lea`+`add`.
  Struct assignment copies byte-wise in 8/4/2/1 chunks.
- Arrays are contiguous; `a[i]` is `base + i * sizeof(elem)`, the index
  scaled with `imul`. An array in value position decays to `lea` of its
  first element.
- Globals: literal-initialized → `.data` (`db/dw/dd/dq`, arrays as value
  lists with a zero-filled tail), uninitialized → `.bss` (`resb`); accessed
  as `[rel name]`.
- String literals → `.rodata` as NUL-terminated byte lists; float literals →
  `.rodata` as `dq` of the IEEE bits.

## 6. Runtime checks

Division, array indexing and pointer dereference are guarded:

```nasm
    test rcx, rcx          ; or: cmp idx, N
    jnz  .Lrtok3           ; jb for bounds
    lea  rdi, [rel rtmsg3] ; "runtime error: ... at line N"
    mov  rsi, <len>
    call rt_error
.Lrtok3:
```

The message string (with the source line resolved at compile time) lives in
`.rodata`. `rt_error` is emitted once per program, only if some check exists;
it uses raw syscalls so it works without libc:

```nasm
rt_error:                  ; rdi = msg, rsi = len
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, 2             ; stderr
    mov rax, 1             ; sys_write
    syscall
    mov rdi, 1
    mov rax, 60            ; sys_exit
    syscall
```

The failure path is never taken on valid programs, so its register clobbers
are harmless and checks cost two instructions each.

## 7. Compiler CLI

```
comp <file> [-o <out.asm>] [-e] [--dump-tokens] [--dump-ast]
```

Without `-o` the output name is the input with the `.asm` extension. `-e`
additionally assembles and links into an executable. The dump flags print the
token stream / AST for debugging.
