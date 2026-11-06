# C++ Interview Notes 07: Common STL Containers

Choosing a container is not only about Big-O. Also consider contiguous storage, cache locality, number of allocations, iterator/reference stability, ordering requirements, and ownership.

Full API lists are on [cppreference](https://en.cppreference.com/w/cpp/container). This note keeps what interviews ask about.

## 1. Quick selection table

| Need | Default choice | Main reason |
|---|---|---|
| General growable sequence | `std::vector<T>` | Contiguous and cache-friendly; usually the first choice |
| Count fixed at compile time | `std::array<T, N>` | No dynamic allocation; the size is part of the type |
| Frequent insert/remove at both ends | `std::deque<T>` | Efficient at both ends, still random access |
| Stable insert/remove at a known iterator | `std::list<T>` | Nodes never move when other nodes are inserted |
| Forward-only traversal, low per-node overhead | `std::forward_list<T>` | Singly linked list |
| Sorted keys / key-value pairs | `std::map<K, V>` / `std::set<T>` | Tree-based; lookup and insert are $O(\log n)$ |
| Fast key lookup only | `std::unordered_map<K, V>` / `std::unordered_set<T>` | Average $O(1)$, no ordering |
| LIFO / FIFO / priority | `std::stack` / `std::queue` / `std::priority_queue` | Adapters express the access model |
| Text | `std::string` | Manages character storage and text operations |

## 2. Sequence containers

### `std::vector`

```cpp
std::vector<int> scores;
scores.reserve(100);          // known count: fewer reallocations
scores.push_back(85);
scores.emplace_back(92);

std::vector<int> filled(2, 400);           // {400, 400}
int raw[] = {16, 2, 77, 29};
std::vector<int> fromArray(std::begin(raw), std::end(raw));

for (int score : scores) {
    std::cout << score << '\n';
}
```

A wrapper around a dynamically allocated C array. Elements are contiguous, which suits indexing, range loops, and APIs that need a contiguous buffer (`data()`). When capacity runs out, it allocates a new buffer, moves or copies the elements, and frees the old buffer; every pointer, reference, and iterator to the old data may become invalid.

| Operation | Complexity | Notes |
|---|---:|---|
| `operator[]` / `at` | $O(1)$ | `at` checks bounds and throws `std::out_of_range`; `[]` out of range is UB |
| `push_back` / `emplace_back` | amortized $O(1)$ | May reallocate |
| insert/erase in the middle | $O(n)$ | Later elements shift |
| `reserve` | depends on move cost | Increases capacity, not size |

### `std::array`

```cpp
std::array<int, 3> coordinates{10, 20, 30};
coordinates.at(0) = 15;
std::cout << coordinates.size();
```

The size is a compile-time constant and the elements live inside the `array` object itself; no `push_back`, no resizing. Compared with a C array it adds `.size()`, iterators, `.at()`, and value semantics (it can be copied and returned, and does not decay to a pointer).

### `std::deque`

A `deque` usually uses segmented buffers: random access is supported, but the elements are not in one contiguous block. `push`/`pop` at either end is $O(1)$; operations in the middle are $O(n)$.

```cpp
std::deque<int> work;
work.push_front(1);
work.push_back(2);
int first = work.front();
```

Inserting at either end invalidates all iterators but keeps references to existing elements valid; inserting in the middle invalidates both. Don't treat it as a drop-in for `vector`'s invalidation rules.

### `std::list` and `std::forward_list`

```cpp
std::list<int> linked{7, 5, 16, 8};
linked.push_front(25);
linked.push_back(13);
if (auto it = std::find(linked.begin(), linked.end(), 16); it != linked.end()) {
    linked.insert(it, 42);      // O(1) once you have the position
}

std::forward_list<int> singly{1, 3};
singly.insert_after(singly.before_begin(), 0);
```

- `list` is a doubly linked list; insert/erase at a known position is $O(1)$. It has no `operator[]`/`at`.
- `forward_list` is a singly linked list: forward traversal only, `insert_after`/`erase_after`, and no `size()`.
- Each node is a separate allocation, so cache locality is much worse than `vector`. Don't assume it is faster just because insert is $O(1)$; finding the position is still $O(n)$.
- Only iterators/references to erased nodes are invalidated.

## 3. Associative containers

### `std::map`, `std::set` and their `multi` variants

Usually implemented as balanced (red-black) trees. Elements are sorted by key; lookup, insert, and erase are $O(\log n)$.

```cpp
std::map<std::string, int> counts;
counts["error"]++;              // operator[] inserts a value-initialized 0 if missing
counts.insert({"warning", 2});  // does nothing if the key exists
counts.insert_or_assign("info", 5);

if (counts.contains("error")) {   // C++20; before that: count() or find()
    std::cout << counts.at("error");
}

for (const auto& [key, value] : counts) {   // sorted by key
    std::cout << key << ": " << value << '\n';
}

std::set<int> unique_values{3, 1, 3, 2};    // iteration order: 1, 2, 3
```

| Container | Header | Keys |
|---|---|---|
| `map` / `multimap` | `<map>` | unique / duplicates allowed, each with a mapped value |
| `set` / `multiset` | `<set>` | unique / duplicates allowed |
| `unordered_map` / `unordered_multimap` | `<unordered_map>` | same, hash-based |
| `unordered_set` / `unordered_multiset` | `<unordered_set>` | same, hash-based |

There is no `<multimap>`, `<multiset>`, `<unordered_multimap>`, or `<unordered_multiset>` header.

**Interview points about `map::operator[]`:**

- It *inserts* a default-constructed value when the key is missing. A lookup like `if (m[k] == 0)` silently grows the map.
- It is non-const, so it can't be used on a `const std::map`. Use `at()` (throws) or `find()`.
- It requires a default-constructible mapped type.

Range queries with `lower_bound` (first element `>= key`) and `upper_bound` (first element `> key`):

```cpp
std::set<int> s;
for (int i = 1; i < 10; ++i) s.insert(i * 10);   // 10 20 30 40 50 60 70 80 90
auto low = s.lower_bound(30);                    // -> 30
auto up  = s.upper_bound(60);                    // -> 70
s.erase(low, up);                                // 10 20 70 80 90
```

For the `multi` variants, `equal_range(key)` returns the `[first, last)` pair of all elements with that key, and `count(key)` may be greater than 1.

Use a tree-based associative container when you need sorted traversal, range queries, or a guaranteed $O(\log n)$ bound.

### `std::unordered_map`, `std::unordered_set`

Hash tables keep no order. Lookup, insert, and erase are average $O(1)$ and can degrade to $O(n)$ in the worst case. When the load factor gets too high, a rehash redistributes all elements.

```cpp
std::unordered_map<std::string, int> cache;
cache.reserve(1024);             // estimate the size to avoid rehashes
cache.emplace("user-1", 42);
cache.try_emplace("user-2", 7);  // C++17: constructs the value only if the key is new

if (auto it = cache.find("user-1"); it != cache.end()) {
    std::cout << it->second;
}
```

`unordered_map` suits lookup-heavy workloads; if you need ordering, `lower_bound`, or range queries, use `map`. A rehash invalidates iterators (but not references to elements), so don't keep iterators across insertions. Custom key types need a hash function and `operator==`.

## 4. Container adapters

Adapters don't expose the full container interface; they wrap a sequence container to provide one access model:

```cpp
std::stack<int> history;
history.push(1);
history.push(2);
int latest = history.top();
history.pop();                   // pop() returns void: read top() first

std::queue<int> requests;
requests.push(10);
int next = requests.front();
requests.pop();

std::priority_queue<int> priorities;
priorities.push(10);
priorities.push(50);
int highest = priorities.top();  // 50
```

| Adapter | Model | Common operations | Default underlying container |
|---|---|---|---|
| `stack` | LIFO | `top`, `push`, `pop` | `deque` |
| `queue` | FIFO | `front`, `back`, `push`, `pop` | `deque` |
| `priority_queue` | max-heap | `top`, `push`, `pop` | `vector` |

For a min-heap, swap the comparator:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;
```

**Interview point**: why does `pop()` return `void`? Returning the element by value could throw while copying it *after* it was already removed, losing it. Separating `top()` and `pop()` keeps the operation exception-safe.

## 5. `std::string` and text storage

`std::string` manages a growable character buffer, with `size()`, `capacity()`, `reserve()`, and `data()`. Short strings may be stored inside the object itself (Small String Optimization, SSO), but SSO is an implementation detail, not a guarantee.

```cpp
std::string message = "hello";
message += " world";             // amortized O(1) per appended char
message.reserve(64);
std::string sub = message.substr(0, 5);    // "hello" (a new string)
std::string_view view = message;  // non-owning; message must stay alive
```

`std::string_view` doesn't own its data: if the original string reallocates or is destroyed, the view dangles.

Splitting a string and converting numbers:

```cpp
#include <sstream>

std::stringstream ss("12 + 30");
std::vector<std::string> tokens;
for (std::string token; std::getline(ss, token, ' ');) {
    tokens.push_back(token);
}
int first = std::stoi(tokens[0]);          // throws std::invalid_argument / out_of_range
std::string text = std::to_string(first + std::stoi(tokens[2]));
```

## 6. `std::pair` and `std::tuple`

```cpp
std::tuple<int, char, std::string> t = std::make_tuple(1, 'a', "x");
std::get<0>(t) = 100;

int n; char c;
std::tie(n, c, std::ignore) = t;    // unpack into existing variables (pre-C++17 style)
auto& [num, ch, str] = t;           // C++17 structured bindings (references into t)
```

## 7. Common comparisons

### Contiguous, segmented, node-based, hash-based

| Kind | Containers | Strengths | Costs |
|---|---|---|---|
| contiguous | `array`, `vector`, `string` | Good cache locality, random access, easy interop with C APIs | Expensive middle inserts; reallocation may invalidate references |
| segmented | `deque` | Efficient at both ends, random access | Not one contiguous buffer |
| node-based | `list`, `map`, `set` | Stable node iterators/references; fast insert at a known position | Many allocations, poor cache locality, pointer overhead |
| hash-based | `unordered_map`, `unordered_set` | Fast average lookup/insert | No order, hash collisions, rehash |

### `map` vs `unordered_map`

| Aspect | `std::map` | `std::unordered_map` |
|---|---|---|
| Structure | balanced tree | hash table |
| Order | sorted by key | no order guarantee |
| Lookup | $O(\log n)$ | average $O(1)$ |
| Range queries | `lower_bound`/`upper_bound` | not suitable |
| Worst case | $O(\log n)$ | $O(n)$ |
| Main risk | node allocations, cache misses | hash collisions, rehash |
| Key requirements | `operator<` (strict weak ordering) | hash + `operator==` |

### `vector` vs `list`

| Aspect | `std::vector` | `std::list` |
|---|---|---|
| Random access | $O(1)$ | not supported |
| Append at the end | amortized $O(1)$ | $O(1)$ |
| Insert at a known position | usually $O(n)$ | $O(1)$ |
| Memory | contiguous | one node per element |
| Cache locality | usually good | usually poor |
| Iterator stability | reallocation invalidates all | other nodes stay valid |

### Iterator invalidation cheat sheet

- `vector`: a reallocation invalidates all iterators/references/pointers. Without one, insert/erase invalidates those at or after the affected position.
- `deque`: insert at either end invalidates iterators (not references); anything in the middle invalidates both. Re-acquire iterators after modifying it.
- `list`/`forward_list`: only erased nodes are invalidated.
- `map`/`set`: only erased elements are invalidated.
- `unordered_map`/`unordered_set`: a rehash invalidates iterators (not references); erased elements are invalidated.

## 8. `push_back` vs `emplace_back`

- `push_back(x)` takes an already-built object and copies it (lvalue) or moves it (rvalue) into the container.
- `emplace_back(args...)` forwards the arguments to the element's constructor and builds it **in place**: no temporary, no extra move.

```cpp
struct Point {
    Point(float x, float y);
    Point(const Point&);            // prints "copied"
    Point(Point&&) noexcept;        // prints "moved"
};

std::vector<Point> v;
v.reserve(4);
v.push_back(Point(1, 2));   // constructed, moved
v.emplace_back(1, 2);       // constructed
```

Interview points:

- **The move constructor must take `T&&`, not `const T&&`.** A `const T&&` overload binds to temporaries and gets chosen, but it can't steal anything from a `const` object, so it is really a copy with a misleading name.
- **Mark the move constructor `noexcept`.** On reallocation, `vector` uses `std::move_if_noexcept`: if the move constructor might throw (and a copy constructor exists), it copies the old elements to keep the strong exception guarantee.
- `reserve(n)` avoids reallocations, and unlike `std::vector<T> v(n)` it doesn't need a default constructor.
- **`emplace_back` can call `explicit` constructors; `push_back` cannot.** That makes it less safe for readability:

  ```cpp
  std::vector<std::vector<int>> nested;
  nested.push_back(1000000);     // compile error: vector(size_type) is explicit
  nested.emplace_back(1000000);  // compiles: appends a vector of 1,000,000 zeros (~4 MB)
  ```

  The second line probably isn't what the author meant, and the compiler won't warn. It doesn't crash; it silently allocates. Prefer `push_back` when you already have an object, and `emplace_back` when constructing from arguments is the point.

Run it: `make run EX=cxx_emplace_back` ([examples/library/emplace_back.cpp](examples/library/emplace_back.cpp))

## 9. References in containers: `std::reference_wrapper`

Containers can't hold references (`std::vector<int&>` is ill-formed): references aren't objects, can't be reseated, and can't be assigned. `std::reference_wrapper<T>` (from `<functional>`) is a copyable, rebindable wrapper that converts implicitly to `T&`:

```cpp
std::vector<int> values{1, 2, 3, 4, 5};
std::vector<std::reference_wrapper<int>> refs(values.begin(), values.end());
for (int& v : refs) {
    v <<= 1;                     // modifies values: {2, 4, 6, 8, 10}
}
```

The wrappers dangle if `values` reallocates or is destroyed, exactly like raw pointers. `std::ref(x)` / `std::cref(x)` create them, which is also how you pass references through `std::thread` and `std::bind`.

Run it: `make run EX=cxx_reference_wrapper` ([examples/library/reference_wrapper.cpp](examples/library/reference_wrapper.cpp))

## 10. Thread safety of containers

A function is *thread-safe* when multiple threads can call it at the same time without data races or corrupted state. Standard containers do no locking. The guarantees ([cppreference: container thread safety](https://en.cppreference.com/w/cpp/container#Thread_safety)) are:

1. **Concurrent calls to `const` member functions** on the same container are safe. For this purpose, some non-const functions also count as read-only: `begin`, `end`, `find`, `lower_bound`, `at`, and (except in associative containers) `operator[]`.
2. **Different elements** of the same container may be modified concurrently by different threads (except `std::vector<bool>`, whose elements share bytes), as long as nothing changes the container's structure.
3. **Anything else needs synchronization**: if one thread modifies the container (`push_back`, `insert`, `erase`, `reserve`, rehash, ...), no other thread may access it at the same time, not even to read. Two threads writing the *same* element is also a data race.

So the precise version of "only const member functions are thread safe" is: *read-only operations may run concurrently with each other; any modification of the container, or of the same element, needs exclusive access.* Also, `map::operator[]` is not read-only: it may insert.

```cpp
std::vector<int> numbers;
int counter = 0;
std::mutex mtx;                 // protects numbers and counter

auto work = [&] {
    std::lock_guard<std::mutex> lock(mtx);
    numbers.push_back(1);       // may reallocate
    ++counter;                  // a plain int ++ from many threads is also a race
};
```

Run it: `make run EX=cxx_container_thread_safety` ([examples/library/container_thread_safety.cpp](examples/library/container_thread_safety.cpp)) (more on mutexes in [02](02_concurrency.md)).

## 11. Selection rules

1. With no special requirement, start with `std::vector`.
2. Size fixed at compile time: `std::array`.
3. Need ordering, range queries, or a predictable $O(\log n)$: `map`/`set`.
4. Key lookup only, order irrelevant: `unordered_map`/`unordered_set`, and `reserve` an estimate.
5. Need stable node iterators *and* many middle inserts/erases: consider `list`.
6. Need queue, stack, or priority behavior: use an adapter to express the intent.
7. Every non-owning iterator, reference, pointer, `reference_wrapper`, and `string_view` needs a clearly longer-lived container or string behind it.
