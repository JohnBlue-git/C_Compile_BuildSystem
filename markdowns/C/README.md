# C Notes Index

This directory covers the C language itself and basic POSIX/system programming. C++ topics such as RAII, the STL, and OOP are in [CXX](../CXX/README.md).

| Note | Contents |
|---|---|
| [00_fundamentals.md](00_fundamentals.md) | Pointers and arrays, dynamic 2D arrays, strings and memory lifetime, `switch`, common undefined behavior |
| [01_data_types_and_sizeof.md](01_data_types_and_sizeof.md) | `struct`/`union`/`enum`, `typedef`, union type punning and endianness, alignment/padding, `sizeof` |
| [02_functions_callbacks_and_arguments.md](02_functions_callbacks_and_arguments.md) | Function pointers, callbacks, pass by value/pointer, pointer-to-pointer |
| [03_strings_memory_and_copy.md](03_strings_memory_and_copy.md) | `memset`/`memcpy`/`memmove`/`strncpy`/`snprintf`, allocation ownership, API checklist |
| [04_qualifiers_and_preprocessor.md](04_qualifiers_and_preprocessor.md) | `const`/`volatile`/`static`, macros vs `inline`, `#include ""` vs `<>`, conditional compilation |
| [05_error_handling_and_signals.md](05_error_handling_and_signals.md) | Segfault causes, `exit`/`abort`/`assert`, `errno`, goto-cleanup, signals and async-signal-safety |
| [06_file_io.md](06_file_io.md) | `fopen` modes, `fgets`/`fputs`/`fprintf`, `fseek`/`ftell`/`rewind`, buffering |
| [Memory](../Memory/README.md) | Stack, heap, storage duration, object lifetime, allocation APIs |
| [Compile](../Compile/README.md) | Preprocessing, translation units, C/C++ linkage, ODR, the linker |

## Suggested order

**00 → 01 → 02 → 03 → 04 → 05 → 06 → Memory → Compile**. For build systems, continue with the [Compile section](../Compile/README.md).

## Examples

Sources are in [examples/](examples/). Each one is also a CTest test (label `c`). Run one with `make run EX=<target>`, or all of them with `make test`.

| Target | What it shows | Note |
|---|---|---|
| `c_pointer_2d` | `int **` 2D array with `malloc` vs `calloc`, cleanup on partial failure | [00](00_fundamentals.md#dynamic-2d-arrays) |
| `c_switch` | `case`, `break`, grouped cases, `default` | [00](00_fundamentals.md#switch) |
| `c_types` | `enum` values, `typedef struct`, `sizeof` with padding | [01](01_data_types_and_sizeof.md) |
| `c_union` | Union type punning and endianness, tagged union | [01](01_data_types_and_sizeof.md) |
| `c_callbacks` | Callback with a `void *context`, pass by value/pointer, `int **` output parameter | [02](02_functions_callbacks_and_arguments.md) |
| `c_copy` | `memset`, `memcpy`, `memmove` with overlap, `strncpy`, `snprintf` truncation | [03](03_strings_memory_and_copy.md) |
| `c_qualifiers` | `const` pointer forms, `static` local and function, `volatile` | [04](04_qualifiers_and_preprocessor.md) |
| `c_preprocessor` | Macro pitfalls, `static inline`, `#`, `##`, predefined macros, `#ifdef NDEBUG` | [04](04_qualifiers_and_preprocessor.md) |
| `c_error_cleanup` | goto-cleanup, `perror`, `assert` | [05](05_error_handling_and_signals.md) |
| `c_signal_handler` | `sigaction`, async-signal-safe handler, `SIG_IGN`, `SIG_DFL` | [05](05_error_handling_and_signals.md) |
| `c_signal_timeout` | `alarm` timeout with `sigprocmask` + `sigsuspend` | [05](05_error_handling_and_signals.md) |
| `c_file_io` | Read `sample.txt`, write/append `output.txt`, file size with `fseek`/`ftell` | [06](06_file_io.md) |
