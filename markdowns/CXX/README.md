# C++ Interview Notes

This directory covers the C++ language, the standard library, and object-oriented
design. The C language, the shared memory model, and the build toolchain are in
[C](../C/README.md), [Memory](../Memory/README.md), and [Compile](../Compile/README.md).

## Notes

| File | Contents |
|---|---|
| [00_core_fundamentals.md](00_core_fundamentals.md) | RAII, `new`/`delete`/placement new, templates, CRTP (static interface, concepts, CRTP vs virtual benchmark), Rule of 0/3/5, vtables/`override`/`final`, const-correctness |
| [01_smart_pointers.md](01_smart_pointers.md) | `unique_ptr`/`shared_ptr`/`weak_ptr`, `make_shared` trade-offs, `shared_ptr` thread safety, reference cycles, `enable_shared_from_this`, callbacks that capture `weak_ptr` |
| [02_concurrency.md](02_concurrency.md) | `thread`/`jthread`, the lock family, `shared_mutex`, `condition_variable`, atomics, C++20 primitives, blocking queue, `async`/`future`/`promise` |
| [03_move_semantics_and_rvo.md](03_move_semantics_and_rvo.md) | Value categories, `std::move`/`std::forward`, move operations, moved-from state, `noexcept` and vector growth, RVO/NRVO |
| [04_lambdas_and_callbacks.md](04_lambdas_and_callbacks.md) | Closure types, capture modes, `mutable`, init-capture, `this` vs `shared_from_this`, `std::function` vs template callbacks, `using` vs `typedef` |
| [05_casts_and_exceptions.md](05_casts_and_exceptions.md) | `static`/`dynamic`/`const`/`reinterpret`/C-style casts, `bit_cast`; `try`/`catch`, `throw;` vs `throw e;`, unwinding, `noexcept`, exception-safety guarantees |
| [06_language_evolution_11_to_20.md](06_language_evolution_11_to_20.md) | What modern C++ aims for; C++11/14/17/20 features, `optional`, `variant`, Concepts |
| [07_stl_containers.md](07_stl_containers.md) | Choosing a container, cheat sheet, `map::operator[]` pitfalls, iterator invalidation, `push_back` vs `emplace_back`, container thread safety |
| [08_oop_solid_and_design_patterns.md](08_oop_solid_and_design_patterns.md) | OOP vocabulary, access control, upcast/downcast, binding, abstract classes, diamond/virtual inheritance, SOLID, design patterns |
| [09_streams_and_file_io.md](09_streams_and_file_io.md) | Stream classes, state flags, the `while (!eof())` anti-pattern, `>>`/`get`/`getline`, leftover input, output streams |
| [10_json_libraries.md](10_json_libraries.md) | nlohmann/json vs Boost.JSON: basics, JSON Pointer, how to choose |

## Suggested reading order

If time is short: **00 → [Memory](../Memory/README.md) → 01 → 03 → 02 → 07 → 08 → 04 → 05 → 06**.
09 and 10 are reference material. For linkage and the build pipeline, read [Compile](../Compile/README.md).

## Examples

Run any example with `make run EX=<target>` from the repository root. Run all of them with
`make test`. Only the C++ examples: `make shell`, then `ctest --preset debug -L '^cxx$'`.

| Target | Shows | Note |
|---|---|---|
| `cxx_raii` | Destructor runs at scope exit and during exception unwinding | 00 |
| `cxx_template` | Function templates and deduction pitfalls | 00 |
| `cxx_crtp` | CRTP static interface checked with a concept | 00 |
| `cxx_crtp_benchmark` | CRTP vs virtual call cost (use `PRESET=release`) | 00 |
| `cxx_smart_pointers` | Ownership transfer, sharing, `weak_ptr::lock`, `bad_weak_ptr` | 01 |
| `cxx_weak_callback` | Stored callback that captures `weak_ptr` vs `shared_ptr` | 01 |
| `cxx_async_future` | `std::async`, the discarded-future trap, exception propagation | 02 |
| `cxx_promise_future` | `promise`/`future` across threads, `broken_promise` | 02 |
| `cxx_move_semantics` | Move constructor/assignment, moved-from state, `noexcept` growth | 03 |
| `cxx_rvo_nrvo` / `cxx_rvo_nrvo_no_elide` | Copy elision on and off (`-fno-elide-constructors`) | 03 |
| `cxx_lambdas` | Capture modes, init-capture, `mutable` | 04 |
| `cxx_callbacks` | Function pointer, member function, `std::function`, template callback | 04 |
| `cxx_casts` | The four C++ casts and what each one refuses | 05 |
| `cxx_exceptions` | Nested `try`/`catch`, `throw;` vs `throw e;` (slicing) | 05 |
| `cxx_optional` | `std::optional`, `reference_wrapper` inside `optional` | 06 |
| `cxx_reference_wrapper` | A container of references | 07 |
| `cxx_emplace_back` | `push_back` vs `emplace_back`, `noexcept` move on reallocation | 07 |
| `cxx_container_thread_safety` | Guarding a shared container with a mutex | 07 |
| `cxx_oop_encapsulation` | Access specifiers, `static`, `friend`, `explicit` | 08 |
| `cxx_oop_inheritance` | Derived copy operations, slicing, hiding vs virtual | 08 |
| `cxx_oop_polymorphism` | Pure virtual base, `vector<unique_ptr<Base>>`, `dynamic_cast` | 08 |
| `cxx_oop_multi_inheritance` | Diamond problem and virtual inheritance | 08 |
| `cxx_oop_singleton_cache` | Meyers singleton holding nested maps | 08 |
| `cxx_stream_state` | `good`/`fail`/`bad`/`eof`, `clear` + `seekg` rewind | 09 |
| `cxx_stream_get_getline` | Character vs line reading | 09 |
| `cxx_stream_input_buffer` | Leftover newline, `ignore` | 09 |
| `cxx_stream_ofstream_put` | Writing with `<<` and `put` | 09 |
| `cxx_json_nlohmann` / `cxx_json_nlohmann_pointer` | nlohmann/json basics and JSON Pointer | 10 |
| `cxx_json_boost` / `cxx_json_boost_pointer` | Boost.JSON basics and `at_pointer` | 10 |

The JSON examples are built only when nlohmann/json and Boost.JSON are found
(the Docker image has both).
