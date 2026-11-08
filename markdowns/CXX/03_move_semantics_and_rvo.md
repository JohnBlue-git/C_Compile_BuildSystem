# C++ Fundamentals 03: Move Semantics and Copy Elision

> How C++ avoids copies: moving resources out of objects that are about to die, and not creating the temporary at all (copy elision).
> Prerequisite: Rule of Five in [00 Core Fundamentals](00_core_fundamentals.md#5-rule-of-zero--three--five).

---

## 1. Value categories in one minute

| Category | Examples | Has an identity? | Can be moved from? |
|---|---|---|---|
| lvalue | `x`, `*p`, `v[0]`, a named rvalue reference parameter | yes | only if you say so (`std::move`) |
| prvalue | `42`, `x + 1`, `Widget()`, a function returning by value | no | it is used to initialize the target directly (C++17) |
| xvalue | `std::move(x)`, `static_cast<T&&>(x)` | yes | yes |

- `T&` binds to lvalues; `const T&` binds to everything; `T&&` (non-deduced) binds to rvalues only.
- **A named rvalue reference is an lvalue.** Inside `Widget(Widget&& other)`, `other` is an lvalue, so passing it on needs `std::move(other)` again.

---

## 2. `std::move` does not move anything

`std::move` is only a cast to an rvalue reference. Its declaration:

```cpp
template <typename T>
constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
// T&& here is a forwarding reference: it binds to lvalues and rvalues.
// The return type strips any reference from T and adds &&.
```

It says "this object may be moved from". The actual work happens when the resulting rvalue picks an overload: a move constructor or move assignment, if one exists. If not, the copy overload (`const T&`) is chosen and you get a copy.

Never declare or define your own `move` in the global namespace or in `std` (the old example re-declared it at global scope); just include `<utility>`.

**A common surprise:** applying `std::move` to a `const` object usually selects the copy constructor:

```cpp
const std::string frozen = "still here";
std::string taken = std::move(frozen);   // const std::string&& → binds to the copy constructor
// frozen is unchanged; no move happened, and no warning by default
```

The same issue appears in a non-`mutable` lambda, where by-value captures are `const` (see [04 Lambdas](04_lambdas_and_callbacks.md#4-init-capture-and-moving-into-a-lambda)).

---

## 3. Move constructor and move assignment

A class that owns a raw resource steals the pointer and leaves the source empty, so the source's destructor does not free it a second time:

```cpp
class Greeter {
public:
  Greeter() : world_(new std::string(" World")) {}
  ~Greeter() { delete world_; }

  Greeter(const Greeter&) = delete;
  Greeter& operator=(const Greeter&) = delete;

  Greeter(Greeter&& other) noexcept : world_(other.world_) {
    other.world_ = nullptr;          // source stays valid: its destructor deletes nullptr (no-op)
  }

  Greeter& operator=(Greeter&& other) noexcept {
    if (this != &other) {
      delete world_;                 // release what we already own
      world_ = other.world_;
      other.world_ = nullptr;
    }
    return *this;
  }

private:
  std::string* world_;
};
```

Notes:
- Members that are already RAII types (`std::string`, `std::vector`, `std::unique_ptr`) move themselves; write `: name_(std::move(other.name_))`, or better, follow the Rule of Zero and do not write the move operations at all.
- Mark move operations `noexcept` when they cannot throw (see section 5).
- After moving a pointer, the source's pointer must be cleared. The old comment said "the source still points to the same memory, you need to ensure no double deletion": that is exactly why `other.world_ = nullptr` is required.

---

## 4. The moved-from state

After a move, the source object still exists and its destructor will run. The standard library guarantees only that its objects are in a **valid but unspecified** state:

- Allowed: destroy it, assign a new value, call functions without preconditions (`clear()`, `size()`, `empty()`).
- Not allowed: rely on its value, or call functions with preconditions (`front()`, `operator[]`, `*p`) before you reset it.

```cpp
std::vector<int> source = {1, 2, 3, 4, 5};
std::vector<int> destination = std::move(source);   // O(1): the buffer pointer is moved
source.clear();                                     // put it into a known state
source.push_back(9);                                // now it can be used again
```

Your own types define their own moved-from state. `Greeter` above documents it: after a move, `hello()` prints only "Hello". `std::unique_ptr` is a special case: a moved-from `unique_ptr` is guaranteed to be null.

Plain members such as `int data = 3;` are **copied** by a move; moving a scalar leaves the source unchanged.

---

## 5. `noexcept` and `std::vector` reallocation

When a `std::vector` grows, it allocates a larger buffer and transfers the elements. To keep the strong exception guarantee (if something throws, the vector is unchanged), it uses `std::move_if_noexcept`: it **moves** only if the move constructor is `noexcept` (or the type is not copyable); otherwise it **copies**.

```text
make run EX=cxx_move_semantics
5. vector growth: noexcept move vs throwing move (1000 push)
  noexcept move : copies = 0, moves = 1023
  may-throw move: copies = 1023, moves = 0
```

So a missing `noexcept` on a move constructor can silently turn every reallocation into a deep copy. Compiler-generated move operations are `noexcept` automatically when all members' moves are.

Related: `push_back(x)` copies an lvalue; `push_back(std::move(x))` moves it; `emplace_back(args...)` constructs the element in place from constructor arguments. `push_back(T(args))` is "construct a temporary, then move it into the vector": one move, never a copy plus a move. See [07 STL Containers](07_stl_containers.md) for `push_back` vs `emplace_back` details.

---

## 6. Forwarding references and `std::forward`

`T&&` is a **forwarding reference** (also called universal reference) only when `T` is deduced in that call (`template <typename T> void f(T&& x)`, or `auto&&`). Reference collapsing then gives:

| Argument | `T` deduced as | Parameter type |
|---|---|---|
| lvalue `std::string` | `std::string&` | `std::string&` |
| rvalue `std::string` | `std::string` | `std::string&&` |

`std::forward<T>(x)` casts back to an rvalue only if the caller passed an rvalue, so one template preserves the caller's value category ("perfect forwarding"):

```cpp
template <typename T>
void relay(T&& arg) {
  sink(std::forward<T>(arg));   // std::move here would always pick the rvalue overload
}
```

Rule: `std::move` for rvalue references (`T&&` with a concrete type), `std::forward<T>` for forwarding references. This is how `emplace_back` and `std::make_unique` pass arguments to constructors.

Run it: `make run EX=cxx_move_semantics` ([examples/core/move_semantics.cpp](examples/core/move_semantics.cpp))

---

## 7. Copy elision: RVO and NRVO

Returning by value does not have to copy or move at all: the compiler can build the result directly in the caller's storage.

| | What is returned | C++17 and later |
|---|---|---|
| **RVO** (return value optimization) | a prvalue: `return Widget();` | **Guaranteed.** Not an optimization any more: a prvalue initializes the target directly, so no temporary exists. Works even if the type has no copy or move constructor. |
| **NRVO** (named RVO) | a named local: `Widget w; ...; return w;` | **Optional**, also in C++20/23. GCC, Clang, and MSVC do it in most simple cases. The type must still be movable or copyable. |

The old notes said "RVO and NRVO are allowed but not required, and in C++17 RVO is mandatory in certain cases". More precisely: since C++17 prvalue elision is not "RVO being mandatory" but a change in the language rules (no temporary is created); NRVO remains a permitted optimization only.

```cpp
Tracer make_rvo()  { return Tracer(); }               // guaranteed: 1 constructor
Tracer make_nrvo() { Tracer obj; return obj; }        // usually: 1 constructor (NRVO)

Tracer make_one_of_two(bool first) {                  // NRVO usually impossible:
  Tracer a; Tracer b;                                 // two candidates can't both live
  if (first) return a;                                // in the return slot
  return b;                                           // → implicit MOVE (not a copy)
}

Tracer pass_through(Tracer t) { return t; }           // parameters are never NRVO'd → implicit move
```

When NRVO does not apply, `return local;` still treats `local` as an rvalue first (**implicit move**, C++11; widened in C++20), so you get a move, not a copy.

### Do not write `return std::move(local);`

```cpp
Tracer pessimized() {
  Tracer t;
  return std::move(t);     // the return expression is no longer the name "t" → NRVO is disabled
}
```

It forces a move where NRVO would have produced zero operations. GCC warns with `-Wpessimizing-move` (in `-Wall`), and with `-Wredundant-move` (in `-Wextra`) for `return std::move(param);`. `std::move` in a return is useful only when returning a member or a subobject, which is not an implicit-move candidate.

### Verifying elision with `-fno-elide-constructors`

GCC and Clang can turn off **optional** elision. The example is built twice from the same source ([examples/core/rvo_nrvo.cpp](examples/core/rvo_nrvo.cpp)):

```bash
make run EX=cxx_rvo_nrvo             # default
make run EX=cxx_rvo_nrvo_no_elide    # same code with -fno-elide-constructors
```

```text
                          default          -fno-elide-constructors (C++17+)
RVO (prvalue):            Constructor      Constructor          ← still elided: guaranteed
NRVO (named local):       Constructor      Constructor
                                           Move Constructor     ← NRVO turned off
two named candidates:     2 ctors + 1 move 2 ctors + 1 move     ← no NRVO either way
```

Before C++17, `-fno-elide-constructors` also added moves for the prvalue case; in C++17 and later it cannot, because there is no temporary to elide. If you see no "Copy Constructor" or "Move Constructor" line, elision happened. Note that GCC performs NRVO even at `-O0`; it is not only a release-mode optimization.

---

## 8. Summary

- `std::move` is a cast; the move constructor does the work; `const` objects are copied.
- Moved-from = valid but unspecified; reset before reuse.
- Move operations `noexcept`, or `std::vector` copies on growth.
- `std::forward` for forwarding references only.
- Return locals by name; never `return std::move(local);`.
- C++17: prvalue elision guaranteed; NRVO optional; otherwise implicit move.
