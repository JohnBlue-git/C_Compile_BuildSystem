# C/C++ Interview Notes 04 — Lambdas and Callbacks

> Lambdas are the everyday way to pass behavior around in C++: algorithms, threads, async tasks, event callbacks. Most interview questions are about **what a capture copies, what it references, and how long that stays valid**.
> C function pointers and `void*` callbacks: see [C 02 Functions and Callbacks](../C/02_functions_callbacks_and_arguments.md).

---

## 1. What a lambda is

A lambda expression creates an object of a unique, unnamed **closure type**. Captures become data members; the body becomes `operator()`.

```cpp
int base = 10;
auto add = [base](int x) { return base + x; };

// Roughly what the compiler generates:
struct __Closure {
  int base;                                        // captured by value
  int operator()(int x) const { return base + x; } // const unless the lambda is mutable
};
```

Consequences:
- Every lambda has its own type. Use `auto` to store one, a template parameter to accept one, or `std::function` to erase the type.
- `operator()` is `const` by default, so by-value captures cannot be modified unless the lambda is `mutable`.
- A lambda with **no captures** converts implicitly to a plain function pointer, so it can be passed to C APIs such as `qsort` or `pthread_create`.

---

## 2. Capture modes

| Capture | Meaning |
|---|---|
| `[]` | nothing captured |
| `[x]` | copy `x` when the lambda is **created** |
| `[&x]` | refer to `x`; sees later changes; dangles if `x` dies first |
| `[=]` | copy every local that the body uses |
| `[&]` | refer to every local that the body uses |
| `[=, &x]` / `[&, x]` | default mode with exceptions |
| `[this]` | copy the `this` **pointer** (members are accessed through it) |
| `[*this]` (C++17) | copy the whole **object** |
| `[y = expr]` (C++14) | init-capture: a new member `y` initialized from any expression |
| `[&y = expr]` | init-capture by reference |

```cpp
int x = 10, y = 20;
auto by_value = [=] { return x + y; };
auto by_ref   = [&] { return x + y; };
x = 100;
by_value();   // 30: copied at creation
by_ref();     // 120: reads the current values
```

Mixing modes:

```cpp
int a = 10, b = 20;
auto mixed = [a, &b] {
  // a += 1;  // error: by-value capture is const in a non-mutable lambda
  b += a;     // OK: b refers to the original
};
```

`const` is preserved through captures:

```cpp
int var1 = 3;
const int& const_ref = var1;
int var2 = 6;
const int* const_ptr = &var2;

auto add = [&const_ref, const_ptr](int num) {
  // const_ref = 1;   // error: refers to const int
  // *const_ptr = 1;  // error: points to const int
  return const_ref + *const_ptr + num;
};
```

`mutable` lets the lambda change **its own copies**; the originals are not affected:

```cpp
auto counter = [n = 0]() mutable { return ++n; };
counter();   // 1
counter();   // 2  (state lives inside the closure object)
```

**Interview point (dangling capture):** a lambda that captures locals by reference must not outlive them.

```cpp
std::function<int()> make_counter() {
  int count = 0;
  return [&count] { return ++count; };   // BUG: count dies when make_counter returns
}
```

Capture by value (or by init-capture) whenever the lambda is stored, returned, or run on another thread.

---

## 3. Lambdas with algorithms

```cpp
std::vector<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};

std::for_each(numbers.begin(), numbers.end(), [](int n) { std::cout << n << ' '; });

int sum = 0;
std::for_each(numbers.begin(), numbers.end(), [&sum](int n) { sum += n; });

auto even = std::count_if(numbers.begin(), numbers.end(), [](int n) { return n % 2 == 0; });
```

Generic lambdas (C++14) use `auto` parameters; each call type instantiates its own `operator()`:

```cpp
auto print = [](const auto& value) { std::cout << value << '\n'; };
print(42);
print("text");
```

C++20 also allows an explicit template parameter list: `[]<typename T>(const std::vector<T>& v) { ... }`.

---

## 4. Init-capture and moving into a lambda

C++11 lambdas could only copy or reference. C++14 init-capture can **move** a move-only or expensive object into the closure:

```cpp
std::function<void()> callback = [] { std::cout << "callback called\n"; };

auto by_copy = [callback] { callback(); };                      // copies the std::function
auto by_move = [callback = std::move(callback)] { callback(); }; // moves it; outer callback is now
                                                                 // valid but unspecified: don't use it

auto owner = std::make_unique<Widget>();
auto task  = [w = std::move(owner)] { w->run(); };              // unique_ptr moved into the closure
```

### Trap: moving out of a capture needs `mutable`

```cpp
std::function<void()> cb1 = /* ... */;

auto outer = [cb1] {                         // cb1 is const inside (operator() is const)
  auto inner = [cb1 = std::move(cb1)] {      // std::move(const T) → const T&& → COPY, silently
    cb1();
  };
  inner();
};

auto outer_fixed = [cb1]() mutable {         // mutable: cb1 is non-const
  auto inner = [cb1 = std::move(cb1)] { cb1(); };   // a real move
  inner();
};
```

The original `use_lambda.cpp` had the non-`mutable` version and labeled it "by move". It compiled and ran, but copied. This is the same `const` + `std::move` trap as in [03 Move Semantics](03_move_semantics_and_rvo.md#2-stdmove-does-not-move-anything).

---

## 5. Capturing `this` vs `shared_from_this()`

Inside a member function, a lambda that uses members captures `this`, a **raw pointer**:

```cpp
class Worker : public std::enable_shared_from_this<Worker> {
public:
  static void static_func();
  void member_func() const;

  void run_now() {
    auto func = [this] {       // fine: func runs before run_now returns
      static_func();           // static members need no this
      member_func();
      ++var_;
    };
    func();
  }

  void run_later() {
    auto task = [self = shared_from_this()] {   // keeps *this alive until task is destroyed
      self->member_func();
    };
    std::thread(task).detach();                 // or post to an event loop / timer
  }

private:
  int var_ = 3;
};
```

| Capture | Object lifetime | Use when |
|---|---|---|
| `[this]` | not extended; dangling if the object dies first | the lambda finishes before the member function returns (algorithms, synchronous calls) |
| `[self = shared_from_this()]` | extended until the lambda is destroyed | async work that must complete; beware of cycles if the object stores the lambda |
| `[weak = weak_from_this()]` + `lock()` | not extended; detects destruction | async work that is pointless once the object is gone (see [01 Smart Pointers](01_smart_pointers.md#5-callbacks-that-capture-weak_ptr)) |
| `[*this]` (C++17) | the lambda owns a copy | the object is small and a snapshot is acceptable |

Details:
- `shared_from_this()` requires the object to already be owned by a `shared_ptr` (otherwise `std::bad_weak_ptr`).
- In a `const` member function, `shared_from_this()` returns `std::shared_ptr<const T>`, so through `self` you can only call `const` members and cannot change `var_`. (The old example's `self->var; // var = 6 invalid` was about this.)
- **C++20 deprecates implicit capture of `this` through `[=]`.** Write `[=, this]` or `[this]` explicitly. `[&]` still captures `this` implicitly.

Run it: `make run EX=cxx_lambdas`

---

## 6. `std::function` and callbacks

`std::function<R(Args...)>` can hold **any** callable with a matching signature: a lambda (with captures), a function pointer, a functor, or a `std::bind` result.

```cpp
using Callback = std::function<int(int)>;

void process_numbers(const Callback& callback) {
  std::cout << "Result: " << callback(9) << '\n';
}

int triple(int n) { return n * 3; }

int var1 = 3, var2 = 6;
process_numbers([&var1, &var2](int num) { return var1 + var2 + num; });   // lambda
process_numbers(triple);                                                  // free function

Multiplier m{5};
process_numbers([&m](int n) { return m.scale(n); });                      // member function
```

### `std::function` vs a template parameter

```cpp
void process_numbers(const std::function<int(int)>& callback);  // type-erased
template <typename F> void process_numbers_fast(F&& callback);   // exact type known
```

| | `std::function` | template parameter `F&&` |
|---|---|---|
| Accepts | any callable with the signature; can be stored in members and containers | any callable; each type makes a new instantiation |
| Call cost | indirect call; usually not inlined | direct call; inlinable |
| Storage | may heap-allocate a large closure (small-buffer optimization avoids it for small ones) | none |
| Code location | one non-template function; can live in a `.cpp` | must be visible in the header |
| Requirement | the callable must be **copyable** (C++23 `std::move_only_function` lifts this) | none |

Use `std::function` to **store** callbacks (event handlers, registries, `std::vector<std::function<...>>`); use a template parameter for a callback that is only **called** inside the function (like the standard algorithms do).

An empty `std::function` is falsy (`if (callback)`); calling it throws `std::bad_function_call`.

Run it: `make run EX=cxx_callbacks`

---

## 7. `using` vs `typedef`

```cpp
typedef std::function<int(int)> Callback;   // C-style
using Callback = std::function<int(int)>;   // modern C++; the same type
```

Both declare the same alias. Reasons to prefer `using`:

- **Readability:** name on the left, type on the right, also for function pointers: `using Handler = void (*)(int);` vs `typedef void (*Handler)(int);`.
- **Alias templates:** only `using` can be templated:
  ```cpp
  template <typename T>
  using Vec = std::vector<T>;
  Vec<int> numbers;
  ```
  With `typedef` you need a wrapper struct (`typename MyAlloc<T>::type`).
- **Consistency** with other modern declarations (`using namespace`, `using Base::Base`).

---

## 8. Quick interview checklist

- `[=]` copies at creation; `[&]` reads current values and can dangle.
- By-value captures are `const` unless `mutable`; `std::move` of them copies.
- `[this]` captures a pointer, not the object; use `shared_from_this()` / `weak_from_this()` for async work; `[=]` capturing `this` is deprecated in C++20.
- A captureless lambda converts to a function pointer.
- `std::function` for storage, templates for speed; empty `std::function` throws `bad_function_call`.
