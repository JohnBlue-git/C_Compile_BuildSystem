# Memory: Interview Notes

This directory covers the memory model and object lifetime that C and C++ share, so stack/heap, allocators, and storage duration are not repeated in each language section.

| Note | Contents |
|---|---|
| [00_stack_heap_and_object_lifetime.md](00_stack_heap_and_object_lifetime.md) | Stack, heap, static storage, C/C++ allocation APIs, object lifetime, callback context lifetime, sanitizer demos, container storage |

## Examples

Sources are in [examples/](examples/). The safe examples are CTest tests (label `memory`). The `memory_bug_*` targets contain deliberate bugs. They are built with `NO_TEST` and are meant to run with the `asan` preset (AddressSanitizer + UBSan). ASan stops them with a non-zero exit status, so `make` also reports an error.

| Target | What it shows | How to run |
|---|---|---|
| `memory_storage_duration` | Addresses of stack, heap, `.bss`, `.data`, `.rodata`, `.text`; a `static` local vs an automatic local | `make run EX=memory_storage_duration` |
| `memory_object_lifetime` | Construction/destruction order for global, automatic, `new`/`delete`, `unique_ptr`, placement `new`, function-local `static` | `make run EX=memory_object_lifetime` |
| `memory_callback_lifetime` | Synchronous callback with a stack context; asynchronous callbacks with heap (owned + destroy function) and static contexts | `make run EX=memory_callback_lifetime` |
| `memory_bug_use_after_free` | ASan: `heap-use-after-free` through a non-owning alias | `make run PRESET=asan EX=memory_bug_use_after_free` |
| `memory_bug_stack_use_after_return` | ASan: `stack-use-after-return`; an async callback keeps a pointer to a dead local | `make run PRESET=asan EX=memory_bug_stack_use_after_return` |
| `memory_bug_heap_overflow` | ASan: `heap-buffer-overflow` from an off-by-one loop | `make run PRESET=asan EX=memory_bug_heap_overflow` |
| `memory_bug_leak` | LeakSanitizer: `detected memory leaks` at exit | `make run PRESET=asan EX=memory_bug_leak` |

Run the safe examples as tests: `make test`, or inside `make shell`: `ctest --preset debug -L memory` (or `--preset asan` to run them under the sanitizers).

## Related notes

- [C notes](../C/README.md): pointers, `malloc`/`free`, strings and copy APIs ([03_strings_memory_and_copy.md](../C/03_strings_memory_and_copy.md)), and callbacks ([02_functions_callbacks_and_arguments.md](../C/02_functions_callbacks_and_arguments.md)).
- [C++ notes](../CXX/README.md): RAII and `new`/`delete` ([00_core_fundamentals.md](../CXX/00_core_fundamentals.md)), [smart pointers](../CXX/01_smart_pointers.md), [lambdas and callbacks](../CXX/04_lambdas_and_callbacks.md), and [STL containers](../CXX/07_stl_containers.md).
- [Compile notes](../Compile/README.md): how the `asan` preset is set up ([02_modern_cmake.md](../Compile/02_modern_cmake.md)).
