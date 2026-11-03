# C/C++ Interview Notes 05 — Casts and Exceptions

> Two topics where C++ gives you precise tools and C habits get you into trouble: named casts instead of `(T)x`, and exceptions with RAII instead of error codes and `goto cleanup`.

---

## Part A: Casts

### 1. Overview

| Cast | Checked at | What it may do | Typical use |
|---|---|---|---|
| `static_cast<T>` | compile time only | conversions the compiler can verify: numeric, `void*` → `T*`, up/down a class hierarchy, enum ↔ integer, explicit constructors/operators | "reverse an implicit conversion"; you know the real type |
| `dynamic_cast<T>` | **run time** (RTTI) | checked downcast or cross-cast in a **polymorphic** hierarchy | you do **not** know the dynamic type |
| `const_cast<T>` | compile time | add or remove `const` / `volatile` only | call a legacy API that is not const-correct |
| `reinterpret_cast<T>` | none | reinterpret bits: pointer ↔ integer, unrelated pointer types | low-level code: hardware registers, serialization, OS handles |
| C-style `(T)x` / `T(x)` | — | tries the casts above in sequence, silently | avoid in C++ |

Named casts are easy to search for (`grep _cast`) and state the intent; the compiler rejects a named cast that would need more power than its name allows.

Run it: `make run EX=cxx_casts`

### 2. `static_cast`

Use it when the conversion is well defined and you already know the type, so no runtime check is needed.

```cpp
double pi = 3.14159;
int truncated = static_cast<int>(pi);          // 3: explicit narrowing

// A C-style API that carries user data as void*
void run_callback(void (*fn)(void*), void* user_data);

void on_job(void* data) {
  auto* job = static_cast<Job*>(data);         // Job* → void* was implicit; this reverses it
}

Job job{17};
run_callback(on_job, &job);
```

Downcasting with `static_cast` (`static_cast<Derived*>(base_ptr)`) compiles for any class in the hierarchy, but if the object is not really a `Derived`, the result is undefined behavior. CRTP relies on this being correct by construction (see [00 CRTP](00_core_fundamentals.md#4-crtp-curiously-recurring-template-pattern)).

### 3. `dynamic_cast`

Use it when the dynamic type is unknown. It checks at runtime:

- To a **pointer**: returns `nullptr` if the object is not of that type.
- To a **reference**: throws `std::bad_cast` (there is no null reference).

```cpp
struct Stmt { virtual ~Stmt() = default; };     // polymorphic: has a virtual function
struct JumpStmt : Stmt { const char* target = "loop_end"; };
struct ExprStmt : Stmt { int value = 7; };

void inspect(Stmt& stmt) {
  if (auto* j = dynamic_cast<JumpStmt*>(&stmt)) {
    // use j->target
  } else if (auto* e = dynamic_cast<ExprStmt*>(&stmt)) {
    // use e->value
  }
}

Stmt& s = expr;
dynamic_cast<JumpStmt&>(s);                     // throws std::bad_cast
```

Rules and costs:
- A **downcast requires a polymorphic source type** (at least one virtual function); otherwise it does not compile:
  ```cpp
  struct Base { };
  struct Derived : Base { };
  Derived d; Base* b = &d;
  dynamic_cast<Derived*>(b);     // error: Base is not polymorphic
  ```
  An upcast (`Derived*` → `Base*`) needs no `dynamic_cast` at all.
- It cannot remove `const` or `volatile`.
- It needs RTTI; builds with `-fno-rtti` (common in embedded and some game code) cannot use it.
- It walks type information at runtime, so it is slower than `static_cast`. A chain of `dynamic_cast` checks is often a sign that a virtual function (or `std::variant` + `std::visit`) would be a better design.

### 4. `const_cast`

`const_cast` only adds or removes `const`/`volatile`. Removing `const` is safe **only if the object itself was not defined `const`**:

```cpp
// OK: a legacy C API that takes char* but never writes through it
std::size_t legacy_length(char* text);
const char* message = "hello";
legacy_length(const_cast<char*>(message));

// OK: the object is not const; only the reference is
int value = 10;
const int& view = value;
const_cast<int&>(view) = 42;                    // value is now 42

// UNDEFINED BEHAVIOR: the object itself is const
const int x = 10;
*const_cast<int*>(&x) = 100;
```

**Interview point.** The old example wrote through `const_cast` to `const int x = 10;` and then printed `x`, which still showed `10`. That is not "const_cast did not work"; it is undefined behavior in action. The compiler is allowed to treat `x` as the constant `10` everywhere (constant folding), while the write may go to a stack slot, to read-only memory (crash), or be removed. Never write to an object that was defined `const`. The example `cxx_casts` only shows this in a comment.

Legitimate uses are rare: old non-const-correct APIs, and implementing a non-const overload in terms of the const one:

```cpp
const T& get(std::size_t i) const { /* ... */ }
T& get(std::size_t i) { return const_cast<T&>(std::as_const(*this).get(i)); }
```

### 5. `reinterpret_cast`

Reinterprets the bit pattern with no check. It is mostly implementation-defined, and it is easy to create undefined behavior.

```cpp
std::uint32_t word = 0x01020304;

// OK: any object may be inspected as bytes through char / unsigned char / std::byte
auto* bytes = reinterpret_cast<const unsigned char*>(&word);
bool little_endian = bytes[0] == 0x04;

// OK: pointer → integer → the same pointer type gives the original pointer back
auto address = reinterpret_cast<std::uintptr_t>(&job);
auto* back   = reinterpret_cast<Job*>(address);

// UB: reading a uint32_t object through a float lvalue breaks strict aliasing
float f = *reinterpret_cast<float*>(&word);

// Well-defined type punning
float g = std::bit_cast<float>(word);           // C++20; or std::memcpy into a float
```

Typical legitimate uses: memory-mapped hardware registers (`reinterpret_cast<volatile std::uint32_t*>(0x4000'0000)`), passing pointers through integer-typed OS or C APIs, byte-level serialization. Object lifetime background: [Memory](../Memory/00_stack_heap_and_object_lifetime.md).

### 6. C-style cast

`(T)expr` (and the functional form `T(expr)`) tries, in order, `const_cast`, `static_cast`, `static_cast` + `const_cast`, `reinterpret_cast`, `reinterpret_cast` + `const_cast`, and uses the first one that compiles. It never does `dynamic_cast`.

```cpp
double pi = 3.14159;
int a = (int)pi;                         // fine: same as static_cast<int>(pi)

const Job cj{1};
Stmt* nonsense = (Stmt*)&cj;             // compiles: silent reinterpret_cast + const_cast
```

The problem is not that it lacks a runtime check; it is that one syntax covers the safe and the dangerous cases. A typo or a changed type can turn a harmless `static_cast` into a `reinterpret_cast` with no warning. In C++ code, use the named casts (GCC's `-Wold-style-cast` flags C-style casts).

**Interview summary:** `static_cast` by default; `dynamic_cast` when the type is unknown and the hierarchy is polymorphic; `const_cast` only for broken APIs and never to write a truly const object; `reinterpret_cast` only for bit-level work, and `std::bit_cast`/`memcpy` for type punning.

---

## Part B: Exceptions

### 7. try / catch / throw basics

```cpp
void inner_function() {
  throw std::runtime_error("Error in inner_function");
}

int main() {
  try {
    inner_function();
  } catch (const std::runtime_error& e) {     // most specific first
    std::cout << "runtime_error: " << e.what() << '\n';
  } catch (const std::exception& e) {         // then the base class
    std::cout << "exception: " << e.what() << '\n';
  } catch (...) {                             // anything else (even throw 42;)
    std::cout << "unknown exception\n";
  }
}
```

- **Throw by value, catch by `const&`.** Catching by value copies the exception and slices derived types.
- Handlers are tried **in order**; the first match wins. Put derived classes before their bases (GCC warns when a handler can never be reached).
- Any type can be thrown, but derive from `std::exception` (usually `std::runtime_error` or `std::logic_error`) so callers can use `what()`.

Standard hierarchy (partial): `std::exception` → `std::logic_error` (`invalid_argument`, `out_of_range`, ...), `std::runtime_error` (`system_error`, `range_error`, ...), `std::bad_alloc`, `std::bad_cast`, `std::bad_weak_ptr`, `std::bad_function_call`, `std::future_error` (a `logic_error`).

### 8. Rethrow: `throw;` vs `throw e;`

A handler can log and pass the exception on:

```cpp
void outer_rethrow() {
  try {
    inner_function();                          // throws ConfigError (derived from runtime_error)
  } catch (const std::exception& e) {
    log(e.what());
    throw;                                     // rethrows the ORIGINAL object: still a ConfigError
  }
}

void outer_throw_copy() {
  try {
    inner_function();
  } catch (const std::exception& e) {
    throw e;                                   // throws a COPY of the static type: a plain std::exception
  }
}
```

`throw e;` copies `e` using its **static** type (`std::exception`), so the `ConfigError` part is sliced off and an outer `catch (const ConfigError&)` no longer matches (`what()` even becomes `"std::exception"`). Always use plain `throw;` to rethrow. A `throw;` with no active exception calls `std::terminate`.

To wrap an exception with extra context while keeping the original, use `std::throw_with_nested` / `std::rethrow_if_nested`. To move an exception between threads, use `std::exception_ptr` (`std::current_exception()`, `std::rethrow_exception()`); `std::future` does this for you (see [02 Concurrency](02_concurrency.md#9-task-based-concurrency-stdasync-stdfuture-stdpromise)).

Run it: `make run EX=cxx_exceptions`

### 9. Stack unwinding and RAII

When an exception leaves a scope, the destructors of all fully constructed local objects run in reverse order. That is why RAII types make code exception-safe without `finally` (see [00 RAII](00_core_fundamentals.md#1-raii-resource-acquisition-is-initialization), example `cxx_raii`).

- If a constructor throws, that object's destructor does **not** run, but its already constructed members and base classes are destroyed. Hold resources in RAII members so a half-built object cannot leak.
- Destructors are implicitly `noexcept`. An exception escaping a destructor calls `std::terminate`, and so does a second exception thrown while unwinding from the first. Never let exceptions leave destructors.

### 10. `noexcept`

- `noexcept` promises that a function does not throw. If an exception escapes anyway, `std::terminate` is called immediately (no unwinding guarantee).
- It is part of the contract used by the library: `std::vector` moves elements on reallocation only if the move constructor is `noexcept` (see [03 Move Semantics](03_move_semantics_and_rvo.md#5-noexcept-and-stdvector-reallocation)).
- `noexcept(expr)` as an operator asks at compile time whether `expr` can throw.

### 11. Exception-safety guarantees

| Guarantee | Meaning | Example |
|---|---|---|
| No-throw | never throws | destructors, `swap`, move operations of most types |
| Strong | if it throws, state is unchanged (commit or rollback) | `std::vector::push_back` (with a copyable or `noexcept`-movable type); copy-and-swap assignment |
| Basic | if it throws, no leaks and invariants hold, but state may have changed | most operations |
| None | anything may happen | avoid |

The `DeepBuffer::operator=` in [00](00_core_fundamentals.md#rule-of-three-a-hand-written-deep-copy) gives the strong guarantee by allocating the replacement before it touches the old data.

### 12. When not to use exceptions

- Expected, frequent failures on a hot path (parsing user input, "not found"): return `std::optional`, an error code, or C++23 `std::expected`.
- Code built with `-fno-exceptions` (some embedded, kernel, and game code), and across C ABI boundaries: an exception must never propagate through a C function or out of a callback called by C code.
- Exceptions cost nothing on the non-throwing path with the common table-based implementations, but throwing is expensive (microseconds), so they are for exceptional situations, not control flow.
