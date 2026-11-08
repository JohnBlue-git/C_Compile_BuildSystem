# C++ Fundamentals 01: Smart Pointers

## Overview

The smart pointers added in C++11 use RAII to manage the lifetime of dynamically allocated objects, which removes the leaks and double frees of manual `new`/`delete`.

| Type | Ownership | Copyable? | Use |
|---|---|---|---|
| `std::unique_ptr` | exclusive | no, move only | the default choice; single owner; zero overhead over a raw pointer (with the default deleter) |
| `std::shared_ptr` | shared (reference-counted) | yes | several owners jointly manage the lifetime; costs a control block and atomic count updates |
| `std::weak_ptr` | none (observer) | yes | observes a `shared_ptr` without extending its lifetime; breaks reference cycles |

Run it: `make run EX=cxx_smart_pointers` ([examples/core/smart_pointers.cpp](examples/core/smart_pointers.cpp))

---

## 1. `std::unique_ptr`

Owns one object exclusively. It cannot be copied; ownership is transferred with `std::move`.

```cpp
#include <memory>

struct Widget {
    int id;
    Widget(int i = 0) : id(i) { std::cout << "Widget " << id << " created\n"; }
    ~Widget() { std::cout << "Widget " << id << " destroyed\n"; }
};

void customDeleter(Widget* w) {
    std::cout << "Custom deleting widget " << w->id << "\n";
    delete w;
}

int main() {
    auto ptr1 = std::make_unique<Widget>(1);      // preferred: make_unique (C++14)

    std::unique_ptr<Widget> ptr2(new Widget(2));  // from new (works, less preferred)

    std::unique_ptr<Widget> ptr3;                 // empty
    ptr3.reset(new Widget(3));                    // assign later

    std::unique_ptr<Widget> ptr4 = std::move(ptr1); // transfer ownership; ptr1 becomes null

    auto arrayPtr = std::make_unique<Widget[]>(2); // array form (needs a default constructor)

    std::unique_ptr<Widget, decltype(&customDeleter)> ptr5(new Widget(5), customDeleter);
}
```

Pitfalls:
- Only wrap memory that came from `new`. `Widget w; std::unique_ptr<Widget> p(&w);` makes the destructor `delete` a stack object: undefined behavior.
- Keeping a raw pointer next to the owner (`Widget* raw = new Widget; std::unique_ptr<Widget> p(raw);`) is legal, but `raw` dangles as soon as `p` releases the object.
- Never give the same raw pointer to two owners (two `unique_ptr`s, or two separately constructed `shared_ptr`s): double free.

**Why prefer `make_unique` over `new`?**
`make_unique<T>(args...)` (C++14) is shorter, never shows a naked `new`, and avoids writing the type twice. It was also introduced to close an **exception-safety** gap:

```cpp
// Before C++17: may leak
foo(std::unique_ptr<T>(new T()), mayThrow());
```

Before C++17, the compiler could evaluate `new T()`, then `mayThrow()`, and only then construct the `unique_ptr`. If `mayThrow()` threw, the raw pointer was not yet owned and leaked. **Since C++17**, each function argument is fully evaluated before the next one starts (the order is still unspecified, but no interleaving), so this specific leak is gone. `make_unique` is still the recommended style.

---

## 2. `std::shared_ptr`

Manages shared ownership with reference counting; the resource is released when the last `shared_ptr` that owns it is destroyed or reset.

```cpp
auto s_ptr1 = std::make_shared<Device>("Device A"); // preferred: one allocation for object + control block

std::shared_ptr<Device> s_ptr2(new Device("Device B")); // two allocations; slower

std::shared_ptr<Device> s_ptr3 = s_ptr1;   // copy: ref count + 1
std::cout << s_ptr1.use_count(); // 2

std::shared_ptr<Device> s_ptr4 = std::move(s_ptr2); // move: ref count unchanged

auto u_ptr = std::make_unique<Device>("Device C");
std::shared_ptr<Device> s_ptr5 = std::move(u_ptr); // unique_ptr converts to shared_ptr

std::shared_ptr<Device> s_ptr6(new Device("Device D"), [](Device* d) {
    delete d; // custom deleter
});
```

**Why is `make_shared` faster than `new` + the `shared_ptr` constructor?**
A `shared_ptr` needs a **control block** that holds the strong count, the weak count, the deleter, and so on.
- `new T()` + `shared_ptr<T>(ptr)`: **two allocations** (one for the object, one for the control block).
- `make_shared<T>(args...)`: **one allocation** that holds both, so fewer heap calls and better locality.

```
new T() + shared_ptr(ptr)              make_shared<T>()
┌──────────┐   ┌───────────────┐       ┌──────────┬───────────────┐
│ T object │   │ control block │       │ T object │ control block │
└──────────┘   └───────────────┘       └──────────┴───────────────┘
 allocation #1    allocation #2                one allocation
```

Trade-offs of `make_shared`:
- The object's destructor runs when the strong count reaches zero, but the single block is freed only when the **weak** count also reaches zero. Long-lived `weak_ptr`s to a large object keep its memory allocated.
- It cannot take a custom deleter (use the constructor, or `std::allocate_shared` for a custom allocator).

### Is `shared_ptr` thread-safe?

Thread safety for `shared_ptr` has three separate parts:

| What | Thread-safe? |
|---|---|
| The **control block** (reference counts) | Yes. Different threads may copy and destroy **different** `shared_ptr` instances that share one object; the counts are updated atomically. |
| The **pointee** (`*p`) | No. Concurrent access to the object follows the object's own rules; you need a mutex or atomics if anyone writes. |
| The **same `shared_ptr` instance** | No. One thread doing `p = other;` or `p.reset()` while another reads or copies `p` is a data race. Use a mutex, or C++20 `std::atomic<std::shared_ptr<T>>`. |

The old note "shared_ptr is thread-safe for reading and writing" is wrong: only the reference counting is.

Cost: every copy and destruction of a `shared_ptr` does an atomic increment/decrement. Pass `const std::shared_ptr<T>&` (or just `T&`) when the callee does not take ownership.

---

## 3. `std::weak_ptr`: breaking reference cycles

A `weak_ptr` observes an object managed by `shared_ptr` but **does not increase the strong count**. It cannot be dereferenced directly: call `.lock()` to get a temporary `shared_ptr` (empty if the object is already gone), or `.expired()` to just check.

### The problem: a reference cycle leaks forever

```cpp
struct Child;
struct Parent {
    std::shared_ptr<Child> child;
    ~Parent() { std::cout << "Parent destroyed\n"; }
};
struct Child {
    std::shared_ptr<Parent> parent; // ← the problem
    ~Child() { std::cout << "Child destroyed\n"; }
};

int main() {
    {
        auto father = std::make_shared<Parent>();
        auto son = std::make_shared<Child>();
        father->child = son;
        son->parent = father;
    } // block ends, but neither destructor runs!
}
```

```
                 ┌────────────┐        ┌────────────┐
   father  ────► │  Parent    │ ─────► │   Child    │
   (local)       │ ref = 2    │ child  │  ref = 2   │
                 │            │ ◄───── │            │ ◄──── son (local)
                 └────────────┘ parent └────────────┘

   Leaving the scope: father and son each drop one reference → both counts stay at 1
   → no count reaches zero → neither object is ever freed (memory leak)
```

### The fix: make the "points back up" reference a `weak_ptr`

```cpp
struct Child {
    std::weak_ptr<Parent> parent; // does not increase Parent's count
};
```

```
                 ┌────────────┐        ┌────────────┐
   father  ────► │  Parent    │ ─────► │   Child    │
   (local)       │ ref = 1    │ child  │  ref = 2   │
                 │            │ ◄╌╌╌╌╌ │            │ ◄──── son (local)
                 └────────────┘ parent └────────────┘
                              (weak_ptr; dashed = not counted)

   Leaving the scope (locals are destroyed in reverse order):
     son    destroyed: Child ref 2 → 1
     father destroyed: Parent ref 1 → 0 → Parent is destroyed
                       → its shared_ptr<Child> member is released: Child ref 1 → 0 → Child is destroyed
```

**Rule of thumb:** ownership usually flows one way (parent owns child). If the child also needs to refer back to the parent, that back reference should almost always be a `weak_ptr`.

### Another classic use: caches

If a cache stores resources (for example game textures) as `shared_ptr`, the cache itself keeps every texture alive even after no player uses it. Store `weak_ptr` in the cache instead:
- Someone is using it → `.lock()` succeeds and returns the existing object.
- Nobody is using it → the object is already destroyed, `.lock()` returns an empty `shared_ptr`, and the cache knows to reload.

---

## 4. `enable_shared_from_this`

Inside a member function, `this` is a raw pointer. Creating `std::shared_ptr<T>(this)` makes a **second, independent control block**, so the object is deleted twice. When an object managed by `shared_ptr` needs to hand out a `shared_ptr` to itself (for example to an async callback), derive from `std::enable_shared_from_this<T>` and call `shared_from_this()`:

```cpp
class Object : public std::enable_shared_from_this<Object> {
public:
  void hello() {
    auto self = shared_from_this();   // shares the existing control block; use_count + 1
    // ... pass self to code that may run later ...
  }
};

auto obj = std::make_shared<Object>();
obj->hello();                           // OK
```

| | raw `this` | `shared_from_this()` |
|---|---|---|
| Ownership | none; does not keep the object alive | shares ownership with the existing owners |
| Safety | dangles if the object dies first | the object lives at least as long as the returned pointer |
| Requirement | none | the object must **already** be owned by a `shared_ptr` |

Calling `shared_from_this()` on an object that is not owned by any `shared_ptr` (on the stack, or owned by a `unique_ptr`, or inside the constructor) throws `std::bad_weak_ptr` since C++17; before C++17 it was undefined behavior. The old example called `hello()` (which uses `shared_from_this()`) on an object owned by a `unique_ptr`, which throws.

Since C++17 there is also `weak_from_this()`, which returns a `weak_ptr` directly.

---

## 5. Callbacks that capture `weak_ptr`

A callback stored in an event loop or timer may run after the object that created it is gone. The capture decides what happens:

| Capture | Effect when the callback runs late |
|---|---|
| `[this]` | dangling pointer: undefined behavior |
| `[self = shared_from_this()]` | keeps the object alive until the callback is destroyed (can also create a cycle if the object stores that callback) |
| `[weak = weak_from_this()]` + `lock()` | does not extend the lifetime; detects that the object is gone and skips the work |

```cpp
std::function<void()> Session::make_weak_callback() {
  std::weak_ptr<Session> weak_self = shared_from_this();
  return [weak_self] {
    auto self = weak_self.lock();       // promotes to shared_ptr for the duration of the call
    if (!self) {
      std::cout << "object gone, skipping\n";
      return;
    }
    self->do_something();               // safe: self keeps the object alive here
  };
}
```

Choose `shared_ptr` capture when the work **must** finish (for example, finishing a write on a socket), and `weak_ptr` capture when the work only makes sense while the object is alive (UI updates, timers of a closed session).

Run it: `make run EX=cxx_weak_callback` ([examples/core/weak_callback.cpp](examples/core/weak_callback.cpp)). More on captures: [04 Lambdas and Callbacks](04_lambdas_and_callbacks.md).

---

## 6. Initialization summary

| Pointer type | Ways to initialize |
|---|---|
| `unique_ptr` | `make_unique` (preferred) / from `new` / `.reset()` / `std::move` from another / custom deleter / array form |
| `shared_ptr` | `make_shared` (preferred) / from `new` / copy (ref + 1) / move (ref unchanged) / from a `unique_ptr` / custom deleter / `shared_from_this()` |
| `weak_ptr` | only from a `shared_ptr` or another `weak_ptr` (or `weak_from_this()`); **not** from a raw pointer, and there is no `make_weak`, because it must observe an existing control block |
