# C/C++ Interview Notes 00 — Core Fundamentals

> Foundations that almost every C++ interview touches.
> The later notes (smart pointers, concurrency, move semantics, lambdas, casts, language evolution, containers, OOP) build on these ideas.

---

## 1. RAII (Resource Acquisition Is Initialization)

**Core idea: bind the lifetime of a resource to the lifetime of an object.**
The constructor acquires the resource (memory, lock, file handle, socket, ...) and the destructor releases it. When the object leaves its scope, its destructor runs, so the resource is released, also when the scope is left by an exception (stack unwinding).

The guarantee has limits: destructors do not run after `std::exit`/`std::abort`/`std::terminate` for objects with automatic storage, and if an exception is never caught it is implementation-defined whether the stack is unwound at all.

```
Manual cleanup (unsafe)              RAII (safe)
─────────────────────────            ─────────────────────────
FILE* f = fopen(...);                {
do_something();  // throws?              std::ifstream f(...);
fclose(f); // never runs                 do_something(); // throws?
                                     } // f's destructor still runs; file closed
```

```cpp
class T {
public:
  T()  { puts("T()"); }
  ~T() { puts("~T()"); }
};

int main() {
  puts("before scope");
  {
    T t;
  }               // ~T() runs here
  puts("after scope");
  // t = {};      // compile error: t no longer exists
}
```

Almost every "safe" abstraction in C++ is an application of RAII:
- `std::unique_ptr` / `std::shared_ptr` (memory)
- `std::lock_guard` / `std::unique_lock` / `std::scoped_lock` (locks)
- `std::fstream` (files)
- `std::jthread` (thread lifetime)

**Interview point:** "Why does C++ not need `try...finally`?" → RAII makes cleanup the destructor's job, so no manual `finally` block is needed.

Run it: `make run EX=cxx_raii` (also shows the destructor running during exception unwinding).

---

## 2. `new` / `delete` and Object Lifetime

`new` does more than allocate raw memory: it allocates and then calls the constructor. `delete` calls the destructor first and then frees the memory. Allocation and release must therefore match:

| Allocation | Correct release | Notes |
|---|---|---|
| `new T` | `delete pointer` | Creates one object; one destructor call |
| `new T[n]` | `delete[] pointer` | Creates an array; destructor called for each element |
| `std::make_unique<T>()` | handled by `std::unique_ptr<T>` | Preferred in modern C++ |
| `std::make_shared<T>()` | handled by `std::shared_ptr<T>` | When ownership is truly shared |

```cpp
Widget* widget = new Widget(42);
delete widget;
widget = nullptr;

Widget* widgets = new Widget[3];
delete[] widgets;
widgets = nullptr;
```

Never mix `new[]` with `delete` or `new` with `delete[]`. A mismatch is undefined behavior: destructors may be skipped, allocator metadata may be corrupted, or the program may crash:

```text
new T       ───────────────> delete pointer
new T[n]    ───────────────> delete[] pointer

new T[n]    ──X────────────> delete pointer       undefined behavior
new T       ──X────────────> delete[] pointer     undefined behavior
```

### `new` failure, `nullptr`, and ownership

A plain `new` that cannot allocate throws `std::bad_alloc`; it does not return `nullptr`. Only the `std::nothrow` form returns a null pointer:

```cpp
try {
  auto* value = new int[large_count];
  delete[] value;
} catch (const std::bad_alloc& error) {
  // allocation failed; error.what() gives diagnostic text
}

int* optional_value = new (std::nothrow) int(42);
if (optional_value != nullptr) {
  delete optional_value;
}
```

A raw pointer says nothing about ownership. An API must state who is responsible for `delete`. If an object has a single owner, prefer:

```cpp
auto value = std::make_unique<int>(42);
auto shared_value = std::make_shared<Widget>(42);
```

Then exception paths cannot skip the release either. `delete nullptr` is a safe no-op, but that does not make a dangling pointer safe: after `delete`, set the owning pointer to `nullptr` and never dereference it again.

### `new` vs `malloc`

| Aspect | `new` / `delete` | `malloc` / `free` |
|---|---|---|
| Works with | C++ objects | raw bytes |
| Constructor/destructor | called | not called |
| On failure | throws `std::bad_alloc`, or `nothrow` returns null | returns `NULL` |
| Pairing | `new` with `delete` | `malloc` with `free` |
| Type | returns a typed pointer | returns `void *`; needs a cast/check |

Never `free` the result of `new`, and never `delete` the result of `malloc`. For a class that holds a `std::string`, a `std::vector`, or virtual functions, memory from `malloc` is not a valid C++ object until one is constructed in it.

### Placement `new`

Placement `new` constructs an object in storage that the caller supplies. It does not allocate, so a plain `delete` on that pointer is wrong. Call the destructor by hand and release the storage the way it was obtained:

```cpp
alignas(Widget) std::byte storage[sizeof(Widget)];
Widget* widget = new (storage) Widget(42);

widget->~Widget();              // end the object's lifetime by hand
// storage is on the stack: never call delete on widget
```

This low-level technique is used by allocators, memory pools, union storage, and container implementations. Application code should prefer RAII containers and smart pointers.

---

## 3. Templates and Compile-Time Polymorphism

A template takes types or constants as parameters, so one piece of code produces different concrete versions at compile time. This is not runtime dispatch: the compiler instantiates the template for each set of arguments it is used with, and type errors usually show up at compile time.

```cpp
template <typename T>
T max_value(T left, T right) {
  return left < right ? right : left;
}

template <typename T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) {
  return N;
}

int values[4];
static_assert(array_size(values) == 4);
```

Common forms:

- **Function template**: reuse function logic for different types, for example `max_value`.
- **Class template**: data structures or resource wrappers parameterized by type, for example `std::vector<T>`.
- **Non-type template parameter**: a compile-time constant as a parameter, for example the array size `N`.
- **Specialization / constraints**: a different implementation for specific types; C++20 concepts constrain template parameters so error messages and interface intent are clearer.

Deduction pitfalls worth knowing:

- `max_value(3, 4.5)` does not compile: `T` is deduced as both `int` and `double`. Write `max_value<double>(3, 4.5)`.
- `max_value("apple", "pear")` deduces `T = const char*` and compares **addresses**, not text. Use `max_value<std::string>(...)`.
- Do not name your own template `max` in code that also does `using namespace std;`: it collides with `std::max`.

Templates usually live in headers because each user must see the full definition to instantiate it. That is the difference from an ordinary function defined in a `.cpp`: with only a declaration visible, you typically get an undefined-reference link error (unless the `.cpp` provides explicit instantiations). See [Compile notes](../Compile/00_compile_and_linkage.md).

Run it: `make run EX=cxx_template`

---

## 4. CRTP (Curiously Recurring Template Pattern)

In CRTP, a derived class passes itself as the template argument of its base class:

```cpp
template <typename Derived>
class Printable {
public:
  void print() const {
    const auto& derived = static_cast<const Derived&>(*this);
    derived.print_impl();
  }
};

class User : public Printable<User> {
public:
  void print_impl() const {
    std::cout << "User\n";
  }
};
```

`Printable<User>` knows `User` at compile time, so `print()` needs no virtual function and no vtable. The main goal is **static polymorphism**: calls are resolved at compile time instead of at runtime through a vtable.

**Benefits:**
- **Performance:** no vtable lookup; since the exact type is known, the call can usually be inlined.
- **Code reuse / mixins:** the base can implement shared behavior on top of derived-specific hooks (for example, an `increment()` in the base that uses `get_value()` from the derived class), operator implementations, policy-based design.
- **Method chaining:** base methods can return `Derived&`, which makes fluent interfaces work without losing the derived type.

### How it works: a static interface with checks

1. `Base<Derived>` exposes the public interface.
2. Inside the base, `this` is converted to `Derived*` with `static_cast`.
3. The compiler binds the call to `Derived`'s method.
4. A `static_assert` with a C++20 concept enforces the interface, so a missing method gives a clear compile-time message instead of a cryptic error deep inside an instantiation.

```cpp
template <typename T>
concept IsAnimal = requires(T v) {
  { v.make_sound() } -> std::same_as<void>;
};

template <typename Derived>
class Animal {
public:
  Animal() {
    // In the constructor, not at class scope: when Animal<Dog> is
    // instantiated as a base class, Dog is still an incomplete type.
    static_assert(IsAnimal<Derived>, "Derived is missing 'void make_sound()'");
  }
  void speak() { static_cast<Derived*>(this)->make_sound(); }
};

class Dog : public Animal<Dog> {
private:
  friend class Animal<Dog>;   // the hook stays private; only the base may call it
  void make_sound() { std::cout << "Woof!\n"; }
};

class SilentFish : public Animal<SilentFish> {};  // SilentFish f; -> static_assert fires
```

The `friend` + private hook keeps `make_sound()` out of the public API. GCC 13 and Clang 18 both accept the concept check on the private hook here; the access context of constraint checks is a subtle area, so if a compiler rejects it, make the hook public.

Run it: `make run EX=cxx_crtp` (uncomment `SilentFish f;` to see the error).

### Limitations

- The base must know the derived type. `Animal<Dog>` and `Animal<Cat>` are unrelated types, so you cannot put a `Dog` and a `Cat` in one `std::vector<Animal*>` and switch types at runtime.
- `static_cast<const Derived&>(*this)` is only correct if `*this` really is a `Derived`. A copy-paste bug such as `class B : public Base<A>` compiles and is undefined behavior. A common guard: give the base a private constructor and `friend Derived;`.
- Each concrete type gets its own instantiation, which can grow the binary; compile times and error messages are usually heavier than with a virtual interface.

### Why CRTP can be faster, and how to measure it

- **No indirection:** a virtual call loads the vptr, loads the function pointer from the vtable, and does an indirect call. A cold vtable can also miss the cache.
- **Inlining (the real gain):** with the exact callee known at compile time, the body is pasted into the caller, which then enables constant folding, vectorization, and loop optimizations across the interface boundary.
- **Branch prediction:** indirect branches are harder to predict than direct ones, but for a call site that always hits the same target, modern CPUs predict it well. The lost inlining usually matters more than the branch.

The example `cxx_crtp_benchmark` calls a trivial `tick()` 50M times through a base pointer and through CRTP. Numbers are **only meaningful with optimization**:

```bash
make run PRESET=release EX=cxx_crtp_benchmark
# virtual: 0.092 s, 1.84 ns/call   (count 50000000)
# CRTP:    3.6e-08 s, ~0 ns/call   (count 50000000)
```

How to read this (and the mistakes in the original benchmark):

- In release, the CRTP loop is inlined and folded into `count += 50000000`; it does not "run 2 million times faster per call". It shows what inlining enables.
- The dynamic type in the example depends on `argc`. If the compiler can see the concrete type (`DerivedVirtual v; BaseVirtual* p = &v;`), it devirtualizes the call, and with an empty body **both loops are deleted**. The old result "Virtual 9e-08 s, CRTP 3e-08 s, 3x faster" was measuring nothing but timer noise.
- In a debug build (`-O0`) nothing is inlined, CRTP pays for an extra call layer, and the result can go either way (the old notes saw CRTP at 0.7x). Debug numbers say nothing about production performance.
- Use `std::chrono::steady_clock` for intervals; for serious work use a framework such as Google Benchmark, which handles warm-up, repetitions, and `DoNotOptimize`.

---

## 5. Rule of Zero / Three / Five

When a class manages a resource directly (especially a raw pointer from `new[]`), copying, moving, and releasing must be designed together:

| Special member function | When it is called | Main responsibility |
|---|---|---|
| destructor | object leaves scope or is `delete`d | release the resource this object owns |
| copy constructor | a new object is created from another object | make an independent copy, or define shared ownership explicitly |
| copy assignment | an existing object is assigned from another object | handle the old resource, then copy the source |
| move constructor | a new object is created from an object about to expire | take over the source's resource; avoid a deep copy |
| move assignment | an existing object takes over from an object about to expire | release the old resource, then take over the source's |

### Shallow copy vs deep copy

Suppose a class holds a pointer `data`:

```text
Shallow copy (copies only the pointer value)

buffer_a.data ─────┐
           ├──> [ 10 ][ 20 ][ 30 ]
buffer_b.data ─────┘

Problem: both objects look like they own data, but they point to the same heap block.
         A change through one is visible through the other; both destructors free it: double free.

Deep copy (copies what the pointer points to)

buffer_a.data ─────> [ 10 ][ 20 ][ 30 ]
buffer_b.data ─────> [ 10 ][ 20 ][ 30 ]

Result: each object owns independent storage; changes and release do not interfere.
```

The compiler-generated copy constructor and copy assignment copy member by member. For a raw pointer that is a shallow copy: no new array is allocated.

| Rule | Content | When |
|---|---|---|
| **Rule of Three** (since C++98) | If a class needs a custom destructor, copy constructor, or copy assignment, it usually needs all three | the class directly owns a raw resource |
| **Rule of Five** (since C++11) | Rule of Three plus move constructor and move assignment | support moves and avoid repeated deep copies of large resources |
| **Rule of Zero** (modern convention) | Write none of the five; let RAII members (`std::vector`, `std::string`, `std::unique_ptr`) manage the resource | the default choice in modern C++ |

```cpp
// Violates the Rule of Three: the compiler-generated copy only copies the data pointer
class ShallowBuffer {
  int* data_;
  std::size_t size_;
public:
  explicit ShallowBuffer(std::size_t size)
    : data_(new int[size]), size_(size) {}
  ~ShallowBuffer() { delete[] data_; }
  // ShallowBuffer copy = original;
  // makes copy.data_ and original.data_ point to the same memory.
};
```

`ShallowBuffer` defines only a destructor, so the compiler still generates a shallow copy. When `copy` and `original` both die, `delete[]` runs twice on the same pointer: a double free.

### Rule of Three: a hand-written deep copy

```cpp
class DeepBuffer {
  int* data_;
  std::size_t size_;

public:
  explicit DeepBuffer(std::size_t size)
    : data_(new int[size]{}), size_(size) {}

  ~DeepBuffer() {
    delete[] data_;
  }

  DeepBuffer(const DeepBuffer& other)
    : data_(new int[other.size_]), size_(other.size_) {
    std::copy(other.data_, other.data_ + size_, data_);
  }

  DeepBuffer& operator=(const DeepBuffer& other) {
    if (this == &other) {
      return *this;              // self-assignment
    }

    int* replacement = new int[other.size_];
    std::copy(other.data_, other.data_ + other.size_, replacement);
    delete[] data_;
    data_ = replacement;
    size_ = other.size_;
    return *this;
  }
};
```

The three functions maintain ownership together: the copy constructor allocates new storage, copy assignment builds the replacement before it swaps out the old storage, and the destructor releases it at the end. Allocating the replacement first also means a failed allocation leaves the original object unchanged.

### Rule of Five: add moves

Since C++11, a class that manages a resource directly should also consider move operations. A move does not copy the array; it transfers ownership of the pointer to the new object and leaves the source in a state that is safe to destroy.

```cpp
class MovableBuffer {
  int* data_;
  std::size_t size_;

public:
  explicit MovableBuffer(std::size_t size)
    : data_(new int[size]{}), size_(size) {}

  ~MovableBuffer() {
    delete[] data_;
  }

  MovableBuffer(const MovableBuffer& other)
    : data_(new int[other.size_]), size_(other.size_) {
    std::copy(other.data_, other.data_ + size_, data_);
  }

  MovableBuffer& operator=(const MovableBuffer& other) {
    if (this != &other) {
      MovableBuffer copy(other);
      swap(copy);
    }
    return *this;
  }

  MovableBuffer(MovableBuffer&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
  }

  MovableBuffer& operator=(MovableBuffer&& other) noexcept {
    if (this != &other) {
      delete[] data_;
      data_ = other.data_;
      size_ = other.size_;
      other.data_ = nullptr;
      other.size_ = 0;
    }
    return *this;
  }

  void swap(MovableBuffer& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
  }
};
```

```text
MovableBuffer source ──data──> [ large array ]
       move constructor
               │
               ▼
MovableBuffer target ──data─┘       source.data == nullptr
```

The source object still exists, but is only guaranteed to be in a "valid but unspecified" state: it must be safe to destroy and to assign to. `noexcept` matters because `std::vector` only moves elements during reallocation when the move constructor cannot throw (otherwise it copies, to keep the strong exception guarantee). Details and a demo: [03 Move Semantics and RVO](03_move_semantics_and_rvo.md).

### Rule of Zero: give ownership to RAII members

```cpp
class GoodBuffer {
  std::vector<int> data_;
public:
  explicit GoodBuffer(std::size_t size) : data_(size) {}
  // No hand-written destructor/copy/move needed.
  // vector provides a correct deep copy and an efficient move.
};
```

Rule of Zero does not mean "never copy". It means the copy/move/release work is delegated to members that already follow RAII. Copying a `std::vector` creates independent elements, moving it usually just transfers the internal buffer; `std::unique_ptr` forbids copying and allows moving, so a class that contains one becomes non-copyable but usually still movable.

**Interview point:** "When does the compiler refuse to generate a copy constructor?" → When the class has a non-copyable member such as `std::unique_ptr`, the implicit copy constructor is defined as deleted. Also know the generation rules:
- A user-declared destructor or copy operation **suppresses the implicit move operations**; "moves" then silently fall back to copies. (Implicit copy is still generated, but that is deprecated.)
- A user-declared move operation makes the implicit copy operations **deleted**.

---

## 6. Virtual Functions and Polymorphism (vtable)

Runtime polymorphism in C++ is implemented (by every mainstream ABI; the standard does not require it) with a **vtable (virtual function table)**:

- For each class with virtual functions, the compiler emits one vtable (an array of function pointers).
- Each object of that class carries a hidden pointer, `vptr`, usually at the start of the object, which points to its class's vtable.
- A call `obj->virtualFunc()` loads the vptr, loads the function pointer at a fixed index, and makes an indirect call. Compared with a direct call that is two dependent loads plus an indirect jump, and, more importantly, the call usually cannot be inlined unless the compiler can devirtualize it.

```
        Base* p = new Derived();
        p->speak();

        p ──> [ Derived object memory ]
                ├─ vptr ──────────────┐
                └─ other data members │
                                      ▼
                            [ Derived's vtable ]
                            ├─ speak()  → Derived::speak
                            └─ ~Base()  → Derived::~Derived
```

**Must-know sub-questions:**
- **Why must a base class destructor be `virtual`?**
  Without it, `delete` through a base pointer to a derived object is undefined behavior; in practice only the base destructor runs and the derived resources leak.
- **What is `override` for?**
  It tells the compiler "this function must override a virtual function". If the signature does not match (for example a wrong parameter type), compilation fails instead of silently declaring a new, unrelated function.
- **What is `final` for?**
  It stops a virtual function from being overridden further, or a class from being inherited. It also lets the compiler devirtualize calls.
- **Pure virtual function (`= 0`) vs abstract class?**
  A class with at least one pure virtual function is abstract: it cannot be instantiated and is used to define an interface.

### History and purpose of `override`

C++98 had no `override` keyword. A derived class wrote a function that looked like the base virtual function and relied on code review, naming conventions, or compiler warnings to confirm it really overrode it. A mistake in `const`, a reference qualifier, a parameter type, or `noexcept` could quietly declare a new function instead.

C++11 added `override`, turning "this must override a base virtual function" into a contract the compiler checks:

```cpp
struct Base {
  virtual void run() const;
  virtual ~Base() = default;
};

struct GoodDerived : Base {
  void run() const override;       // correct override
};

struct BadDerived : Base {
  void run() override;             // compile error: missing const
};
```

`override` does not change dispatch and does not make a function virtual on its own; it only asks the compiler to verify the override. Use it on every function meant to override, and make the base destructor virtual when objects are deleted through a base pointer. `final`, also from C++11, forbids further overriding or inheritance.

### Template/CRTP vs virtual/`override`

| Aspect | Template / CRTP | Virtual / `override` |
|---|---|---|
| When dispatch happens | compile time | run time |
| Type requirement | concrete type known at compile time | concrete type hidden behind a base interface |
| Call cost | usually inlinable; no vtable dispatch | possible vptr/vtable indirect call; rarely inlined |
| Binary/compile cost | may grow with one instantiation per type | one shared implementation; more stable compile times |
| Containers and plugins | not for heterogeneous runtime collections | fits `std::vector<std::unique_ptr<Base>>` and plugin ABI boundaries |
| Interface errors | found at instantiation or concept check | `override` checks the signature at the declaration |
| Typical use | static interface, mixin, policy, zero-cost abstraction | runtime strategy, GUI/widget hierarchy, dependency inversion |

Rule of thumb: if the algorithm and the concrete types are known at compile time, template/CRTP gives static polymorphism; if implementations are swapped at runtime or different derived types are mixed, use a virtual interface and protect it with `override`.

---

## 7. const-correctness

```cpp
const int a = 1;          // a itself cannot change
int* const p1 = &x;       // the pointer p1 cannot change, *p1 can (const pointer)
const int* p2 = &x;       // *p2 cannot change, p2 can (pointer to const; same as int const* p2)
const int* const p3 = &x; // neither can change

class Widget {
    int value;
public:
    int get() const { return value; }       // const member function: promises not to change the object
    // only const member functions can be called on a const object
};
```

Reading tip: read right to left. `int* const` is "const pointer to int"; `const int*` is "pointer to const int".

**Interview point:** "What is `mutable` for?" → It lets a member change inside a `const` member function. Typical uses are caches and internal locks, for example the `mutable std::shared_mutex cache_mutex;` in the [DNS cache example](02_concurrency.md#4-stdshared_mutex-c17-many-readers-one-writer): `get_ip() const` still has to lock it.

Removing `const` with `const_cast` and then writing to an object that was **defined** `const` is undefined behavior; see [05 Casts and Exceptions](05_casts_and_exceptions.md).

---

## Further reading: Memory

Stack, heap, storage duration, object lifetime, and container storage apply to both C and C++, so they live in the [Memory section](../Memory/00_stack_heap_and_object_lifetime.md).
