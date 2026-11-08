# C++ Design 08: Object-Oriented Programming and Design Patterns

First the OOP vocabulary and the C++ mechanics behind it (access control, binding, casts, multiple inheritance). Then SOLID and the common patterns.

A pattern is a shared name for a recurring design problem. It is not a goal in itself, and you should not add indirection just to apply one. Make ownership, dependency direction, and invariants clear first, then pick the smallest abstraction that works.

## 1. OOP vocabulary

| Term | Meaning |
|---|---|
| Class | A type that groups data and the functions that operate on it |
| Object | An instance of a class |
| Encapsulation | Bundle data with the methods that use it, and hide the implementation behind an interface so invariants can't be broken from outside |
| Inheritance | A derived class reuses and extends a base class ("is-a") |
| Polymorphism | Different derived classes behave differently behind the same base interface |
| Abstraction / interface | The set of operations a type promises, without the implementation |
| Overriding | A derived class replaces a base class **virtual** function with the same signature (mark it `override`) |
| Overloading | Several functions with the same name but different parameter lists in the same scope; resolved at compile time |
| Hiding | A derived class declares a function with the same name as a base function that is *not* overridden; the base overloads become invisible through the derived type |

Benefits, if designed well: reuse, and code that is easier to maintain and extend. Encapsulation also organizes the implementation and stops callers from depending on its details.

## 2. Access control, `static`, `friend`, `explicit`

| Specifier | Accessible from |
|---|---|
| `public` | everyone |
| `protected` | the class itself, its friends, and derived classes (a derived class may access protected members only through objects of its own type, not through an arbitrary `Base&`) |
| `private` | the class itself and its friends |

Members of a `class` are `private` by default; members of a `struct` are `public`. That's the only difference between the two keywords.

Inheritance mode caps the access of inherited members:

| Inheritance | base `public` becomes | base `protected` becomes | Relationship |
|---|---|---|---|
| `public` | public | protected | is-a; upcast allowed everywhere |
| `protected` | protected | protected | implementation detail |
| `private` | private | private | "implemented in terms of" (prefer composition) |

Base `private` members are never accessible in the derived class, whatever the mode.

**`static`** has different meanings depending on where it appears:

- **Static data member**: one copy per class, shared by all objects. Before C++17 it needs one out-of-class definition (`int A::count = 0;`); since C++17 write `inline static int count = 0;` in the class. A `static constexpr` member is implicitly inline.
- **Static member function**: no `this`, so it can't access non-static members directly; call it as `A::f()`.
- **Static local variable**: static storage duration (lives until program exit), initialized the first time control passes through it; thread-safe since C++11.
- **`static` at namespace scope**: internal linkage (visible only in that translation unit). See [C](../C/README.md) for the C meaning.

**`friend`** grants a specific function or class access to private and protected members. Friendship is not inherited and not transitive.

**`explicit`** on a constructor blocks implicit conversions: with `explicit Account(int)`, `Account a = 3;` is an error, `Account a(3);` is fine.

A `const` data member must be initialized in the member initializer list; it can't be assigned in the constructor body.

Run it: `make run EX=cxx_oop_encapsulation` ([examples/library/oop/encapsulation.cpp](examples/library/oop/encapsulation.cpp))

## 3. Inheritance mechanics

- **Construction order**: virtual bases first, then direct bases in declaration order, then members in declaration order, then the constructor body. The order in the member initializer list doesn't matter (GCC warns with `-Wreorder` when it differs). Destruction runs in reverse.
- **Derived copy operations**: if you write a copy constructor or copy assignment in a derived class, call the base version explicitly (`Derived(const Derived& o) : Base(o) {...}`, `Base::operator=(o);`). Otherwise the base part is default-constructed or not assigned. Better: follow the Rule of Zero and let the compiler generate them ([00](00_core_fundamentals.md)).
- **Virtual destructor**: a base class that is deleted through a base pointer needs a `virtual` destructor; otherwise `delete base_ptr` is undefined behavior.
- **Object slicing**: copying a derived object into a base *object* (`Shape s = rect;`) keeps only the base part. Virtual calls on `s` then use the base implementation.

### Upcast and downcast

**Upcast** (derived to base) is implicit and safe for public inheritance. Through a pointer or reference there is no slicing:

```cpp
Derived derived;
Base* pb = &derived;          // implicit upcast
pb->func();                   // virtual: calls Derived::func
pb->Base::func();             // qualified call: forces Base::func

void process(const Base& b) { b.func(); }   // dynamic dispatch through a reference
process(derived);                            // calls Derived::func
```

**Downcast** (base to derived) must be explicit. Never use a C-style cast for it:

```cpp
Base* pb = getShape();
if (auto* pd = dynamic_cast<Derived*>(pb)) {   // checked at run time
    pd->derivedOnly();                          // pb really pointed to a Derived
}                                               // otherwise pd == nullptr
Derived& rd = dynamic_cast<Derived&>(*pb);      // reference form throws std::bad_cast

auto* fast = static_cast<Derived*>(pb);         // no check: UB if *pb is not a Derived
```

- `dynamic_cast` needs a polymorphic base (at least one virtual function) and costs a run-time type check.
- `static_cast` is fine only when you *know* the dynamic type (for example, CRTP).
- A C-style cast `(Derived*)pb` silently becomes a `static_cast` (or worse, a `reinterpret_cast` when the types are unrelated or the base is inaccessible); it hides mistakes. More in [05](05_casts_and_exceptions.md).

Needing many downcasts is usually a design smell: move the behavior into a virtual function instead.

Run it: `make run EX=cxx_oop_inheritance` ([examples/library/oop/inheritance.cpp](examples/library/oop/inheritance.cpp))

## 4. Polymorphism, binding, and abstract classes

| | Static (early) binding | Dynamic (late) binding |
|---|---|---|
| When resolved | compile time | run time, through the vtable |
| Applies to | non-virtual functions, overloads, templates, any call on an object (not through pointer/reference) | `virtual` functions called through a pointer or reference |
| Chosen by | the **static** type of the expression | the **dynamic** type of the object |

```cpp
Shape* base = &rect;
base->describe();   // non-virtual: Shape::describe, even though *base is a Rectangle
base->area();       // virtual: Rectangle::area
```

A **pure virtual function** is declared with `= 0`:

```cpp
virtual double area() const = 0;
```

- **Abstract class**: has **at least one** pure virtual function (declared or inherited and not overridden). It can't be instantiated, but you can use pointers and references to it. A pure virtual function may still have a definition, called with `Base::f()`.
- **Interface class** (a convention, not a language feature): only pure virtual functions plus a virtual destructor, no data members.
- A derived class stays abstract until it overrides every pure virtual function.

How virtual dispatch works (vptr, vtable, cost): see [00](00_core_fundamentals.md).

Run it: `make run EX=cxx_oop_polymorphism` ([examples/library/oop/polymorphism.cpp](examples/library/oop/polymorphism.cpp))

## 5. Multiple inheritance and the diamond problem

C++ allows a class to inherit from several bases; Java and C# allow only one base class (plus any number of interfaces). In practice, multiple inheritance is mostly used to **implement several interfaces** (pure abstract classes) or to add **mixins**. Inheriting state from several bases is rarer and harder to reason about.

**Diamond problem**: `Dad` and `Mom` both derive from `Grand`, and `Kid` derives from both.

```
 class Dad : public Grand            class Dad : virtual public Grand
 (two Grand subobjects)              (one shared Grand subobject)

   Grand     Grand                          Grand
     |         |                           /     \
    Dad       Mom                        Dad     Mom
      \       /                            \     /
         Kid                                 Kid
```

Without virtual inheritance, `Kid` contains **two** `Grand` subobjects:

- `kid.x()` is ambiguous (compile error); you must write `kid.Dad::x()` or `kid.Mom::x()`.
- `Grand* g = &kid;` is ambiguous too.
- Static members of `Grand` are still shared (one per class, not per subobject).

**Virtual inheritance** (`class Dad : virtual public Grand`) shares **one** `Grand` subobject:

- The **most-derived class** constructs the virtual base. `Dad`'s and `Mom`'s `Grand(...)` initializers are ignored when building a `Kid`; if `Kid` doesn't name `Grand(...)` in its initializer list, `Grand`'s default constructor is used.
- Virtual bases are constructed before all non-virtual bases.
- The cost: an extra indirection to reach the base, and a more complex object layout.

Run it: `make run EX=cxx_oop_multi_inheritance` ([examples/library/oop/multi_inheritance.cpp](examples/library/oop/multi_inheritance.cpp))

## 6. SOLID in C++

The five SOLID principles keep object-oriented code easy to maintain and extend.

| Principle | In C++ | Warning signs |
|---|---|---|
| **S**ingle Responsibility: one reason to change | One class per kind of change; separate I/O, domain rules, and storage | God class; constructor takes a dozen external services |
| **O**pen/Closed: open for extension, closed for modification | Extend behavior through interfaces, composition, injected strategies | Every new type means editing a giant `switch` |
| **L**iskov Substitution: a derived object must work wherever the base is expected | Derived types keep the base's preconditions and postconditions; an interface must not promise what implementations can't do | An `override` that throws "not supported" or changes the meaning |
| **I**nterface Segregation: clients don't depend on methods they don't use | Small, focused pure-virtual interfaces | Implementers forced to write empty methods |
| **D**ependency Inversion: depend on abstractions, not concretions | High-level policy depends on an interface; the concrete implementation is injected at the composition root | Domain code that directly `new`s a database or network client |

Formally, Liskov says: if a property *q(x)* holds for every object *x* of type *T*, then *q(y)* must hold for every object *y* of a subtype *S*.

## 7. Strategy: inject a varying algorithm

Supports OCP and DIP: the caller depends on `PricingPolicy`, and a new discount strategy doesn't change `Checkout`. For small, stateless strategies a lambda or function object is enough; don't build a class hierarchy just for the pattern.

```cpp
class PricingPolicy {
public:
    virtual ~PricingPolicy() = default;
    virtual double price(double subtotal) const = 0;
};

class Checkout {
public:
    explicit Checkout(std::unique_ptr<PricingPolicy> policy)
        : policy_(std::move(policy)) {}
    double total(double subtotal) const { return policy_->price(subtotal); }
private:
    std::unique_ptr<PricingPolicy> policy_;
};
```

## 8. Factory Method / Abstract Factory: centralize creation of compatible objects

A factory isolates creation details at the composition boundary. The factory itself shouldn't become a giant `switch`; if the set of types is configurable, use a registry or injected creator functions. Abstract Factory fits when you need a *family* of compatible products, for example a `Button` and a `Window` that must belong to the same platform.

## 9. Adapter: isolate existing or third-party APIs

An adapter converts an external API into the narrow interface your domain needs (ISP, DIP). Don't let third-party types leak through the adapter into the core model; otherwise switching vendors still means changes everywhere.

## 10. Decorator: add responsibilities without changing the class

Wrap a component behind the same interface to stack logging, retry, metrics, or caching. This usually fits OCP better than inheritance. Keep ownership and error-propagation rules consistent across decorators.

## 11. Observer: event notification and lifetimes

Observer fits one-to-many notification, but registering raw pointers easily leaves dangling callbacks. In modern C++, define an explicit unsubscribe, or return a subscription token whose RAII destructor unregisters. If events cross threads, also define synchronization and which thread runs the callbacks ([04](04_lambdas_and_callbacks.md)).

## 12. Command: turn a request into an object

A command object can carry undo, queuing, retry, and auditing. It suits editors, job queues, and transactional operations. A command should own the data it needs, not hold references that may expire.

## 13. State: replace conditional explosion with state transitions

When a class keeps growing `if (state == ...)` branches, the State pattern separates each state's behavior and transition rules. If there are few states and the rules are stable, an `enum` plus an explicit transition table is usually simpler.

## 14. Template Method vs composition

Template Method fixes the algorithm's skeleton in a base class and lets subclasses override steps. It relies on inheritance, so check LSP carefully. If the "algorithm" is just a few replaceable steps, prefer Strategy/policy composition to reduce coupling.

## 15. Singleton: know why to avoid it

A singleton hides dependencies, makes testing hard, and brings global state and initialization-order problems. When a resource must be shared, usually create one instance in `main` (the composition root) and pass it by reference or interface. Consider a controlled singleton only for truly process-wide runtime resources.

If you do need one, the **Meyers singleton** is the idiomatic form:

```cpp
class Cache {
public:
    static Cache& instance() {
        static Cache cache;   // created on first call; thread-safe since C++11
        return cache;
    }
    Cache(const Cache&) = delete;              // no copies
    Cache& operator=(const Cache&) = delete;   // (move operations are then not declared)
private:
    Cache() = default;
    std::map<std::string, std::map<unsigned, std::map<unsigned, std::string>>> companies_;
};
```

- Only the *initialization* is thread-safe. The member functions still need their own mutex if several threads call them.
- The instance is destroyed at program exit, in reverse order of construction. Another static object's destructor that uses it after that is undefined behavior.
- The example also shows type aliases (`using EmployeeList = std::map<...>`) for nested maps and structured bindings to iterate them.

Run it: `make run EX=cxx_oop_singleton_cache` ([examples/library/oop/singleton_cache.cpp](examples/library/oop/singleton_cache.cpp))

## 16. Choosing an abstraction

1. Identify the axis of change: creation, algorithm, notification, state, or an external API.
2. Try a function, composition, or a simple data table before a class hierarchy.
3. Check SOLID: do dependencies point at abstractions, does substitution hold, are interfaces small?
4. Explain ownership, thread safety, error handling, and how you would substitute test doubles.
