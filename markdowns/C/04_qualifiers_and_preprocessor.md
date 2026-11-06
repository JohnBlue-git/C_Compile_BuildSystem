# C Notes 04: Qualifiers (`const`, `volatile`, `static`) and the Preprocessor

## `const`

Read the declaration from right to left: `const` applies to the thing on its left (or on its right when it comes first).

| Declaration | Can change `*p`? | Can change `p`? |
|---|---|---|
| `const int *p` (same as `int const *p`) | No | Yes |
| `int *const p` | Yes | No |
| `const int *const p` | No | No |

`const` means "read-only through this name", not "constant forever": a `const int *` can point to a non-const `int` that someone else changes.

### Casting away `const`

```c
const int cx = 3;
int *p = (int *)&cx;  /* compiles: the cast removes the qualifier */
*p = 6;               /* undefined behavior: cx was defined const */
```

The old note said "a const can be modified indirectly through a pointer". That is wrong: modifying an object that was *defined* `const` is UB. In practice the compiler may have folded `cx` to `3` (so reads still show 3), or the object may be in read-only memory (`.rodata`) and the write crashes. Casting away `const` is only valid when the original object is not const, for example a legacy API that takes `char *` but does not write.

## `volatile`

`volatile` tells the compiler that the object can change, or that an access has side effects, outside the code it can see. Every read and write in the source must become a real memory access; the compiler may not keep the value in a register or remove "redundant" accesses.

- Use it for memory-mapped hardware registers, for a `volatile sig_atomic_t` flag written by a signal handler ([05](05_error_handling_and_signals.md)), and for local variables modified between `setjmp` and `longjmp`.
- It does **not** bypass the CPU cache, and it does **not** make an access atomic or order it against other threads. For threads, use `_Atomic`/`<stdatomic.h>` or a mutex.
- It prevents optimizations, so it has a performance cost.
- `volatile const` is valid and useful: a read-only status register that the program must not write but the hardware can change, for example `extern volatile const uint32_t STATUS_REG;`.

## `static`

`static` has three different meanings in C:

| Where | Effect |
|---|---|
| Local variable in a function | **Static storage duration**: initialized once (before the program starts, with a constant), keeps its value between calls |
| Function or variable at file scope | **Internal linkage**: visible only in this translation unit (`.c` file) |
| Array parameter: `void f(int a[static 10])` | Promise to the compiler that `a` points to at least 10 elements (C99) |

```c
static int next_id(void) {          /* internal linkage */
    static int counter = 0;         /* initialized once */
    return ++counter;               /* 1, 2, 3, ... */
}
```

A `static` variable defined in a header gives *each* `.c` file that includes it its own private copy, which is rarely what you want. To share one variable, declare it `extern int g_count;` in the header and define it `int g_count;` in exactly one `.c` file. See [Compile: linkage and ODR](../Compile/00_compile_and_linkage.md).

Run it: `make run EX=c_qualifiers` ([examples/qualifiers.c](examples/qualifiers.c))

## The preprocessor

The preprocessor transforms the source text before compilation proper. It handles lines that start with `#`. Look at its output with `gcc -E file.c`. The full build pipeline is in [Compile](../Compile/00_compile_and_linkage.md).

| Feature | Directives / operators |
|---|---|
| Macro expansion | `#define`, `#undef` |
| Header inclusion | `#include` |
| Conditional compilation | `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `defined(...)` |
| Line control | `#line` (used by code generators to map back to the original file) |
| Diagnostics | `#error`, `#warning` (standard in C23) |
| Implementation-specific | `#pragma` (for example `#pragma once`, `#pragma pack`) |
| Stringification / token pasting | `#x` turns an argument into a string literal; `a##b` joins two tokens |

### `#include "file"` vs `#include <file>`

- `#include "file"`: GCC first searches the directory of the file that contains the `#include` (not the current working directory), then `-iquote` directories, then the same list as `<>`.
- `#include <file>`: searches `-I` directories, then `-isystem` and the standard system directories (for example `/usr/include`).

The standard leaves the exact search to the implementation; it only requires that `""` falls back to the `<>` search. Use `""` for project headers and `<>` for system and third-party headers. Protect every header with an include guard (`#ifndef MY_HEADER_H` / `#define MY_HEADER_H` / `#endif`) or `#pragma once`.

### Macro vs `inline` function

Both aim to remove function-call overhead by putting the body at the call site. Both can make code larger when the body is big or used in many places, and the extra size (instruction cache pressure) can cost more than the call saved.

| | Function-like macro | `inline` function |
|---|---|---|
| Expanded by | Preprocessor, as text, before compilation | Compiler, if it decides to |
| Type checking | None | Yes |
| Argument evaluation | Once per use in the body (side effects repeat) | Exactly once |
| Debugging | Hard (no symbol, errors point at the expansion) | Easy (a normal function at `-O0`) |
| Guarantee | Always expanded | Only a hint: the compiler can refuse, and it also inlines functions without the keyword at `-O2` |

```c
#define SQUARE(x) ((x) * (x))   /* parenthesize each argument and the whole result */
#define BAD_SQUARE(x) x * x     /* BAD_SQUARE(1 + 2) -> 1 + 2 * 1 + 2 == 5 */

static inline int cube(int x) { return x * x * x; }
```

- `SQUARE(i++)` expands to `((i++) * (i++))`: two unsequenced modifications of `i`, which is UB.
- Wrap multi-statement macros in `do { ... } while (0)` so they behave like one statement after an `if`.
- **C inline linkage pitfall:** in C (unlike C++), `inline int cube(int x) { ... }` alone in a `.c` file is only an *inline definition*. If the compiler decides not to inline the call (always at `-O0`), it emits a call to an external `cube` that does not exist, and the link fails with "undefined reference". Use `static inline` (the usual choice, also in headers), or put the `inline` definition in a header and `extern inline int cube(int);` in exactly one `.c` file.

### Conditional compilation

Select code at build time, for example by platform, byte order, or debug mode:

```c
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    /* little-endian code (GCC/Clang predefined macros) */
#else
    /* big-endian code */
#endif

#ifndef NDEBUG
    fprintf(stderr, "debug: x = %d\n", x);   /* removed in builds with -DNDEBUG */
#endif
```

Macros can come from the command line (`gcc -DLOG_LEVEL=2`) or from CMake (`target_compile_definitions`). Useful predefined names: `__FILE__`, `__LINE__`, `__STDC_VERSION__` (`201710L` for C17), `__DATE__`, and the predefined identifier `__func__` (not a macro).

Run it: `make run EX=c_preprocessor` ([examples/preprocessor.c](examples/preprocessor.c))
