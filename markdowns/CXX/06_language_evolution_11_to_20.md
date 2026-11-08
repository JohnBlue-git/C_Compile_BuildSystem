# C++ Language Evolution 06: C++11, C++14, C++17, and C++20

> Each language revision adds features in response to practical needs. Understanding the problem a feature addresses makes its design and appropriate use easier to remember than memorizing version numbers.

---

## What modern C++ pursues

Modern C++ is more than "C with objects". It aims for:

- **Type safety**: let the type system reject mistakes (`nullptr`, `enum class`, `std::optional`, `std::variant`, `std::span`).
- **Errors at compile time, not at run time**: `static_assert`, `constexpr`, Concepts.
- **Zero-cost abstractions**: generic code (templates, lambdas, ranges) that compiles down to what you would write by hand.
- **Functional-style tools**: lambdas, algorithms, ranges pipelines.

The two most important changes are **RAII everywhere** (smart pointers, lock guards) and **move semantics**. Before C++11, code was full of `new`/`delete` and expensive copies, and you had to manage every resource's lifetime by hand. RAII plus move semantics made ownership something the type system tracks, and that changed how C++ is written. Many other features are syntactic sugar on top, but the standard defines them as new *semantic* concepts (value categories, guaranteed copy elision, and so on), so knowing what they mean still matters.

See [00](00_core_fundamentals.md) for RAII, [01](01_smart_pointers.md) for smart pointers, and [03](03_move_semantics_and_rvo.md) for move semantics.

---

## C++11: the foundation of modern C++

| Feature | Problem it solves |
|---|---|
| Move semantics: `std::move`, `&&` (rvalue references) | Avoid expensive deep copies; "steal" the resources of temporaries instead ([03](03_move_semantics_and_rvo.md)) |
| Smart pointers `unique_ptr` / `shared_ptr` / `weak_ptr` | RAII memory management, no leaks from a forgotten `delete` ([01](01_smart_pointers.md)) |
| `auto` / `decltype` | Type deduction, less boilerplate (especially iterator types) |
| Lambdas `[](){}` | Define anonymous functions in place, for STL algorithms (`sort`, `find_if`) and callbacks ([04](04_lambdas_and_callbacks.md)) |
| `nullptr` | Replaces the `NULL` macro (really `0`); type-safe, never picked as an integer in overload resolution |
| `override` / `final` | State the intent to override; the compiler catches signature typos ([00](00_core_fundamentals.md)) |
| `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic` | A portable, standard threading model instead of OS-specific pthreads ([02](02_concurrency.md)) |
| `std::future` / `std::promise` / `std::async` | Hand a result (or exception) from one thread to another ([02](02_concurrency.md)) |
| `std::unordered_map` / `std::unordered_set` | Hash tables join the standard library |
| `std::array`, `std::tuple` | Type-safe fixed-size array; packing several values together |
| Alias templates (`using`) | `template <typename T> using Vec = std::vector<T>;` A readable `typedef` that can also be a template |
| Range-based `for` | `for (auto& x : v) x *= 2;` |

### Move semantics example
```cpp
std::vector<int> makeBigVector() {
    std::vector<int> v(1000000, 42);
    return v; // NRVO usually removes the copy entirely; if not, C++11 moves instead of copying
}
std::vector<int> data = makeBigVector();
```

---

## C++14: filling C++11's gaps

| Feature | Problem it solves |
|---|---|
| **`std::make_unique`** | C++11 gave us `make_shared` but forgot `unique_ptr`. C++14 adds it and closes an exception-safety hole ([01](01_smart_pointers.md)) |
| Generic lambdas (`auto` parameters) | `[](auto x, auto y){ return x + y; }`: a lambda that works like a template |
| Lambda init-capture | `[p = std::move(ptr)]{ ... }`: move a move-only object into a lambda, or capture a computed value ([04](04_lambdas_and_callbacks.md)) |
| **Relaxed `constexpr`** | See the section below |
| `std::shared_timed_mutex` (C++14), `std::shared_mutex` (C++17) | Reader-writer lock: many readers or one writer ([02](02_concurrency.md)) |
| Binary literals, digit separators | `int mask = 0b1010'1100;` (the separator is `'`, not `_`) |
| `std::exchange` | Read the old value and set a new one in one step; common in move constructors |

### What relaxed `constexpr` changed

**C++11**: a `constexpr` function may contain only a single `return` statement. No loops, no modified local variables.
```cpp
// C++11: recursion instead of a loop
constexpr int factorial11(int n) {
    return n <= 1 ? 1 : n * factorial11(n - 1);
}
```

**C++14**: loops, `if`/`switch`, and modification of local variables are allowed (if the object's lifetime starts inside the evaluation).
```cpp
constexpr int factorial14(int n) {
    int result = 1;
    for (int i = n; i > 1; --i) {
        result *= i; // not allowed in C++11, fine in C++14
    }
    return result;
}
constexpr int value = factorial14(5); // evaluated at compile time
```

**Compile-time or run-time?** A `constexpr` function is guaranteed to be evaluated at compile time only when its result is needed in a constant expression (a `constexpr` variable, an array bound, a template argument, or `static_assert`). Otherwise it may run at run time like a normal function, though the optimizer may still fold it. C++20 adds `consteval` for functions that must be evaluated at compile time.

---

## C++17: readability and performance

| Feature | Problem it solves |
|---|---|
| Structured bindings (`auto [x, y] = ...`) | Unpack a tuple/pair/struct directly, no `std::get<0>` |
| `if`/`switch` with an initializer | `if (auto it = m.find(k); it != m.end())`: limits the temporary's scope to the `if` |
| `std::string_view` | Read-only string slice, no allocation, avoids needless string copies |
| `std::optional` / `std::variant` | See below: type-safe replacements for "sentinel values" and C-style unions |
| `inline` variables | Define a `static` data member in the class (`inline static int count = 0;`) or a global in a header without ODR violations |
| `std::filesystem` | Portable paths and file operations |
| `std::scoped_lock` | One-line deadlock avoidance when locking several mutexes ([02](02_concurrency.md)) |
| Guaranteed copy elision | Returning a prvalue (`return T{...};`) never copies or moves ([03](03_move_semantics_and_rvo.md)) |
| Parallel STL (`std::execution::par`) | Let `std::sort`, `std::transform`, and others run on several cores |

### `std::optional`: a value that may be absent

Why use it:

- **It states intent**: the type says "there may be no value", so the reader doesn't have to guess.
- **No sentinel values**: no special `-1`, `nullptr`, or empty string that means "nothing".
- **No separate error flag**: one return value instead of `bool ok` plus an out-parameter.

```cpp
std::optional<std::string> findName(bool found) {
    if (found) return "John Doe";
    return std::nullopt;
}

auto name = findName(true);
if (name) {                         // same as name.has_value()
    std::cout << *name;             // unchecked: UB if empty
    std::cout << name.value();      // checked: throws std::bad_optional_access if empty
}
std::cout << name.value_or("<none>");
name.reset();                       // now empty
```

Passing an optional follows the usual rules: by value, the function gets its own copy; by `std::optional<T>&`, it changes the caller's optional.

**`std::optional<T&>` is not allowed** before C++26. To return or pass an optional *reference*, use `std::optional<std::reference_wrapper<T>>` (or a plain `T*`, where `nullptr` means "none"):

```cpp
void appendSuffix(std::optional<std::reference_wrapper<std::string>> target) {
    if (target) {
        target->get() += " (processed)";   // modifies the caller's string
    }
}

std::string s = "Hello";
appendSuffix(std::ref(s));     // s == "Hello (processed)"
appendSuffix(std::nullopt);
```

`std::reference_wrapper` is also how you store references in containers. See [07](07_stl_containers.md#9-references-in-containers-stdreference_wrapper).

Run it: `make run EX=cxx_optional` ([examples/library/optional.cpp](examples/library/optional.cpp))

**Choosing an error representation:** use `optional` when "no result" is a normal outcome, such as a lookup miss. If the caller needs to know why an operation failed, use an exception or `std::expected<T, E>` (C++23).

### `std::variant`: a type-safe union

A C-style `union` doesn't remember which member it currently holds and doesn't call constructors or destructors, so it is easy to misuse. `std::variant<T1, T2, ...>` holds exactly one of its alternatives at a time, and access is checked.

```cpp
#include <variant>
#include <string>

std::variant<int, double, std::string> v;

v = 42;
std::cout << v.index() << "\n"; // 0 (the first alternative, int)

v = "Hello";                    // now holds a std::string
if (std::holds_alternative<std::string>(v)) {
    std::cout << std::get<std::string>(v).size() << "\n"; // safe access
}

try {
    std::get<int>(v); // v does not hold an int: throws
} catch (const std::bad_variant_access& e) {
    std::cout << "type mismatch: " << e.what() << "\n";
}

// std::visit: generic "pattern matching"
std::visit([](auto&& arg) { std::cout << arg << "\n"; }, v);
```

| Operation | Meaning |
|---|---|
| `v.index()` | Zero-based index of the alternative currently held |
| `std::get<T>(v)` / `std::get<i>(v)` | Access by type or index; throws `std::bad_variant_access` on mismatch |
| `std::get_if<T>(&v)` | Pointer to the value, or `nullptr` on mismatch; never throws |
| `std::holds_alternative<T>(v)` | Is the current alternative `T`? Never throws |
| `std::visit(visitor, v)` | Generic access; the standard way to do type-safe pattern matching |
| `v.valueless_by_exception()` | Rare "empty" state, reached when a constructor throws during assignment |

**When to use it**: one variable that holds different types at different times (a parsed JSON value, per-state data in a state machine). `variant` handles construction and destruction for you and forces type-safe access.

---

## C++20: the biggest revision in a decade

| Feature | Problem it solves |
|---|---|
| **Concepts** | See below: replace unreadable SFINAE with named, semantic constraints on template parameters |
| **Ranges** (`std::ranges`, `views`) | Pass a container directly to an algorithm (no `begin()`/`end()` pair) and build lazy pipelines with `|` |
| **Coroutines** (`co_await`, `co_yield`, `co_return`) | Write asynchronous code that reads like synchronous code (the standard gives the language mechanism; libraries such as Boost.Asio provide the task types) |
| **Modules** (`import`) | Replace textual `#include` copying; faster builds, no macro leakage ([Compile](../Compile/README.md)) |
| **Three-way comparison `<=>`** | Write one operator; the compiler derives `<`, `<=`, `>`, `>=`. A defaulted `<=>` also implies a defaulted `==` (and `!=`) |
| `std::jthread`, `std::stop_token`, semaphores, latches, barriers | See [02](02_concurrency.md) |
| `std::span` | Like `string_view` but for any contiguous memory: a non-owning view with a size |
| `std::format` (C++20), `std::print` (C++23) | Type-safe, fast replacement for `printf` and long `std::cout` chains |
| `consteval`, `constinit` | Force compile-time evaluation; force static (not dynamic) initialization |

### Concepts in depth

Before C++20, template parameters were constrained with SFINAE (`std::enable_if`): obscure syntax and error messages dozens of lines long. Concepts let you constrain template parameters **with named semantic requirements**.

```cpp
// A concept: T must be an integral type
template <typename T>
concept Integral = std::is_integral_v<T>;

// Constrain a template with it
template <Integral T>
T add(T a, T b) { return a + b; }

add(3, 4);      // OK: int satisfies Integral
add(3.0, 4.0);  // error, and the message says "Integral not satisfied" instead of template noise
```

Concepts combine with logical operators:
```cpp
template <typename T>
concept SignedIntegral = Integral<T> && std::is_signed_v<T>;
```

**Concepts and `static_assert` operate at different stages.** A `static_assert` inside a template fires only **after** the template is selected and instantiated, often deep inside its body. A Concept is checked **during overload resolution**: unsatisfied candidates are removed, diagnostics point closer to the call site, and constrained overloads can coexist (the most constrained one wins).

---

## Quick reference

| Feature | Standard | One-liner |
|---|---|---|
| Move semantics | C++11 | Performance: no deep copies |
| `make_unique` | C++14 | Fills C++11's gap, exception safety |
| Lambda init-capture | C++14 | Move objects into lambdas |
| `optional` / `variant` | C++17 | Type-safe "maybe" and type-safe union |
| Structured bindings | C++17 | Syntax sugar for returning several values |
| Concepts | C++20 | Constrain templates, replace SFINAE, readable errors |
| Ranges | C++20 | Containers straight into algorithms, lazy pipelines |
| Coroutines | C++20 | Async code that reads like sync code |
| Modules | C++20 | Replace `#include`, faster builds |
