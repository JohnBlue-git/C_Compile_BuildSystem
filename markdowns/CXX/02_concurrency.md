# C/C++ Interview Notes 02 — Concurrency / Multithreading

> A favorite deep-dive topic for tooling, backend, and infrastructure roles. Interviewers usually go from "can you use the API" to "do you understand the underlying behavior and pitfalls".

---

## 1. Vocabulary

- **Data race**: two or more threads access the same memory location without synchronization, and at least one access is a write. In C++ this is **undefined behavior**: the program has no defined meaning at all, and the optimizer may assume it never happens.
- **Race condition**: a logic flaw where correctness depends on timing or ordering. Even with no data race (every access locked), the wrong order can still break logic, for example another thread slipping in between "check balance" and "withdraw".
- They differ: a data race is an unsynchronized access; a race condition is an order-sensitive logic bug. A data race is almost always a bug, but having no data race does not mean having no race condition.

---

## 2. Thread management: `std::thread` vs `std::jthread`

### The `std::thread` trap

If a `std::thread` object is destroyed while still **joinable** (neither `.join()` nor `.detach()` was called), its destructor calls `std::terminate()` and the program dies.

```cpp
void risky() {
    std::thread t([]{ /* do some work */ });
    if (someCondition) return; // early return! t is destroyed while joinable → std::terminate()
    t.join();
}
```

### `std::jthread` (C++20) fixes this

- Its destructor **joins automatically**, so forgetting no longer crashes.
- Built-in **cooperative cancellation** with `std::stop_token` / `std::stop_source`: you can ask the thread to stop cleanly instead of killing it.

```cpp
#include <thread>
#include <stop_token>

void worker(std::stop_token st) {
    while (!st.stop_requested()) {
        // keep working...
    }
    std::cout << "stop requested, exiting cleanly\n";
}

int main() {
    std::jthread t(worker); // the stop_token is passed in automatically
    std::this_thread::sleep_for(std::chrono::seconds(1));
    t.request_stop();       // ask the thread to stop
} // t joins automatically when it leaves scope
```

**Interview answer:** in modern C++ (especially C++20 and later), prefer `std::jthread` unless you specifically need the raw `std::thread` behavior.

---

## 3. The lock family: `std::mutex` / `std::lock_guard` / `std::unique_lock` / `std::scoped_lock`

### 3.1 `std::mutex`: the raw resource

It has no notion of scope. Lock it and forget to unlock it, and every other thread waits forever.

```cpp
mtx.lock();
if (error) { mtx.unlock(); return; } // forget this line and you have a permanent deadlock
mtx.unlock();
```

### 3.2 `std::lock_guard`: the lightest RAII wrapper

Locks in the constructor, unlocks in the destructor. **No unlocking midway, no deferred locking.** Use it to "lock one mutex until the end of the scope".

```cpp
void safe_increment() {
    std::lock_guard<std::mutex> lock(mtx); // locks on construction
    shared_data++;
    if (shared_data > 100) return; // safe: the destructor unlocks
} // safe: unlocks at the end of the scope
```

### 3.3 `std::unique_lock`: more flexible, slightly heavier

On top of RAII it supports:
- **Deferred locking** `std::defer_lock`: do not lock in the constructor; call `.lock()` later.
- **Try-locking** `std::try_to_lock`: try once; if the lock is busy, give up without blocking.
- **Adopting a held lock** `std::adopt_lock`: assume the caller already holds the lock; only manage the unlock.
- **Unlock / relock midway**, **ownership transfer** (it is movable), and, most important, **use with `std::condition_variable`** (because `cv.wait()` must unlock and relock, which `lock_guard` cannot do).

```cpp
std::unique_lock<std::mutex> lock(resource_mutex, std::defer_lock); // not locked yet
// ... preparation that needs no lock ...
if (need_write) {
    lock.lock(); // lock only when needed
    // critical section
}
// unlocks on scope exit if it was locked; otherwise does nothing

std::unique_lock<std::mutex> lock2(resource_mutex, std::try_to_lock);
if (lock2.owns_lock()) { /* got the lock */ }
else { /* lock is busy; do something else instead of waiting */ }
```

| Locking strategy tag | Behavior | Typical use |
|---|---|---|
| `std::defer_lock` | do not lock now; call `.lock()` later | with `std::lock()` for several mutexes, conditional locking |
| `std::try_to_lock` | try once; give up without blocking if busy | real-time code (for example a render loop) that must not wait |
| `std::adopt_lock` | assume the caller already holds the lock; manage only the unlock | integrating with C-style APIs or external locking |

### 3.4 `std::scoped_lock` (C++17): the fix for multiple locks

**Background (the deadlock trap):** two threads each need `mtx1` and `mtx2` but take them in opposite order:

```
Thread 1: locks mtx1 → waits for mtx2
Thread 2: locks mtx2 → waits for mtx1
           ↓
        each waits for the other forever (deadlock)
```

**Before C++17** (`std::lock` + `adopt_lock`; verbose and easy to forget `adopt_lock`):
```cpp
std::lock(mtx1, mtx2); // locks both with a deadlock-avoidance algorithm
std::lock_guard<std::mutex> l1(mtx1, std::adopt_lock);
std::lock_guard<std::mutex> l2(mtx2, std::adopt_lock);
```

**Since C++17:** `std::scoped_lock` does both steps in one line, and as a variadic template it accepts any number of mutexes:

```cpp
void transfer_money(std::mutex& mtx1, std::mutex& mtx2) {
    std::scoped_lock lock(mtx1, mtx2); // locks both with deadlock avoidance
    // critical section...
} // both unlock automatically
```

`std::scoped_lock` with a single mutex fully replaces `std::lock_guard`. **Modern guideline: default to `std::scoped_lock`; use `unique_lock` only when you need its extra features (deferred locking, condition variables, manual unlock).**

### 3.5 Summary

| | `std::mutex` | `std::lock_guard` | `std::unique_lock` | `std::scoped_lock` (C++17) |
|---|---|---|---|---|
| Role | the resource itself | light RAII wrapper | flexible RAII wrapper | multi-lock RAII wrapper |
| Deferred locking | — | no | yes | no |
| Unlock midway | manual | no | yes | no |
| Lock several mutexes safely | needs `std::lock` | no | needs `std::lock` | **built in** |
| Works with condition_variable | — | no (cannot unlock midway) | **yes (the only choice)** | no |
| When to use | almost never alone | older code / single lock | condition variables, deferred locking | **default choice** |

---

## 4. `std::shared_mutex` (C++17): many readers, one writer

When data is read often and written rarely, let many readers in at the same time, but give a writer exclusive access.

- With `std::shared_lock`: many readers can hold it at once (shared lock).
- With `std::unique_lock`: a writer holds it exclusively (`std::lock_guard` also works on the write side, but `unique_lock` is the common convention for consistency and flexibility).

> Common misunderstanding: `std::shared_mutex` itself has no "read mode" or "write mode". The mode comes from the wrapper: `shared_lock` gives shared (read) semantics, `unique_lock` (or `lock_guard`) gives exclusive (write) semantics. **If every access uses `lock_guard`, it behaves exactly like a plain `std::mutex`, and readers no longer run in parallel.**

```cpp
class DNSCache {
    std::map<std::string, std::string> dns_table;
    mutable std::shared_mutex cache_mutex;
public:
    void update_entry(const std::string& domain, const std::string& ip) {
        std::unique_lock<std::shared_mutex> lock(cache_mutex); // exclusive: write
        dns_table[domain] = ip;
    }
    std::string get_ip(const std::string& domain) const {
        std::shared_lock<std::shared_mutex> lock(cache_mutex); // shared: read, many threads at once
        auto it = dns_table.find(domain);
        return it != dns_table.end() ? it->second : "Not Found";
    }
};
```

**Lock compatibility:**

| Current holder | New request: `shared_lock` (read) | New request: `unique_lock` (write) |
|---|---|---|
| nobody | granted immediately | granted immediately |
| readers | granted (readers coexist) | blocks until all readers release |
| a writer | blocks | blocks |

**When is it worth it?** Only when reads far outnumber writes and each read takes a while. If reads and writes are both frequent, or reads are very short, the extra bookkeeping in `std::shared_mutex` makes it slower than a plain `std::mutex` + `std::lock_guard`.

---

## 5. `std::condition_variable`: signaling between threads

The trio: a `condition_variable` (the signal) + a `mutex` (protects the shared state) + a `bool` flag or predicate (protects against spurious and lost wakeups).

**Why must `cv.wait()` use `std::unique_lock`, not `lock_guard`?**
`wait()` must **atomically** unlock the mutex and go to sleep, then relock the mutex when it wakes up. `lock_guard` cannot unlock midway, so only `unique_lock` can do this.

```cpp
std::mutex mtx;
std::condition_variable cv;
bool ready = false;
int shared_data = 0;

void worker_thread() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return ready; }); // see below
    std::cout << "data ready: " << shared_data << "\n";
}

int main() {
    std::thread worker(worker_thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    {
        std::lock_guard<std::mutex> lock(mtx); // only modifies data here; lock_guard is enough
        shared_data = 42;
        ready = true;
    }
    cv.notify_one();
    worker.join();
}
```

### The lifecycle of `cv.wait()`

```
worker thread                          main thread
─────────────                          ─────────────
lock mtx
cv.wait(lock, pred)
  │
  ├─ pred() == true? yes → continue immediately
  │
  └─ pred() == false → unlock mtx automatically, sleep
                                       lock mtx (worker released it)
                                       set shared_data, ready = true
                                       unlock mtx
                                       cv.notify_one()  ─────┐
                                                             │
      woken up ◄─────────────────────────────────────────────┘
      relock mtx automatically
      check pred() again; true → wait() returns, shared_data is safe to read
```

### Why always pass a predicate instead of a bare `cv.wait(lock)`?

- **Spurious wakeup**: a waiting thread can wake up even though nobody called `notify()`.
- **Lost wakeup**: if `notify_one()` runs before the consumer enters `wait()`, that notification is gone and a bare `wait()` may sleep forever.

A predicate such as `[]{ return ready; }` is a `while` loop: it checks the condition before sleeping (so an earlier notification is not lost, because the flag is already set) and again after every wakeup. Equivalent form:
```cpp
while (!ready) cv.wait(lock);
```

The flag must be changed while holding the mutex, even if it is an atomic; otherwise the change can land between the waiter's check and its sleep, and the wakeup is lost.

---

## 6. `std::atomic`: lock-free synchronization

`std::atomic<T>` protects data with CPU atomic instructions (for example compare-and-swap or locked read-modify-write) instead of a lock.

**`std::atomic<int>` vs a `std::mutex` around an `int` (common interview question):**

| | `std::mutex` + `int` | `std::atomic<int>` |
|---|---|---|
| Mechanism | uncontended: a user-space atomic operation; contended: the thread may sleep in the kernel (futex on Linux) | a single atomic instruction (for example `lock xadd` on x86) |
| Under contention | waiting threads are suspended and woken later (context switches) | never sleeps; the cache line bounces between cores, and CAS loops may retry |
| Performance | extra cost, especially when contended | much faster for simple operations on one variable |
| Use for | complex critical sections (several variables, several steps) | one simple variable: flags, counters |

```cpp
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed); // lock-free increment
```

Notes: `std::atomic<T>` is not guaranteed to be lock-free for every `T` (check `is_lock_free()` / `is_always_lock_free`); and two atomic operations in a row are not atomic together (`if (x.load() == 0) x.store(1);` is a race condition; use `compare_exchange_strong`).

---

## 7. New synchronization primitives in C++20

- **`std::counting_semaphore` / `std::binary_semaphore`**: lightweight counting signals; often cheaper and simpler than a mutex + condition_variable pair. Typical uses:
  - Limit how many threads use a resource at once (connection pool size, worker quotas).
  - Signal between threads without owning a lock: unlike a mutex, a semaphore can be released by a thread other than the one that acquired it.
  - Note: the standard does **not** guarantee that `release()` is async-signal-safe. From a signal handler, use a `volatile std::sig_atomic_t` or lock-free atomic flag, or POSIX `sem_post`, which is async-signal-safe.
- **`std::latch`**: a single-use countdown; when it reaches zero all waiters are released; cannot be reset.
- **`std::barrier`**: a reusable meeting point; when all threads arrive, an optional completion function runs and the next phase starts.
- **`std::atomic_ref`**: atomic operations on an existing object that is not itself declared `atomic`.
- **`std::jthread` and `std::stop_token`**: see section 2.

```cpp
std::counting_semaphore<3> pool_slots(3); // at most 3 threads inside at once
void access_resource() {
    pool_slots.acquire(); // waits here if no slot is free
    // use the resource...
    pool_slots.release();
}
```

---

## 8. Classic exercise: a thread-safe bounded blocking queue

This one tests mutex + condition_variable + a capacity limit at once, and is a staple of tooling and infrastructure interviews.

```cpp
template <typename T>
class BoundedBlockingQueue {
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable not_full, not_empty;
    size_t capacity;
public:
    explicit BoundedBlockingQueue(size_t cap) : capacity(cap) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [&] { return q.size() < capacity; }); // wait while full
        q.push(std::move(item));
        lock.unlock();
        not_empty.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [&] { return !q.empty(); }); // wait while empty
        T item = std::move(q.front());
        q.pop();
        lock.unlock();
        not_full.notify_one();
        return item;
    }
};
```

**Design point:** two condition variables (`not_full` for producers, `not_empty` for consumers) are more efficient than one cv with `notify_all()`, because fewer threads wake up for nothing. Unlocking before `notify_one()` saves the woken thread from immediately blocking on the mutex.

---

## 9. Task-based concurrency: `std::async`, `std::future`, `std::promise`

Threads and condition variables are low level. When you only want "run this and give me the result later", use a **future**: a one-shot channel that carries either a value or an exception from a producer to a consumer.

| Piece | Role |
|---|---|
| `std::future<T>` | the receiving end; `get()` waits and returns the value (or rethrows the exception); one-shot and move-only |
| `std::shared_future<T>` | copyable; several consumers can `get()` the same result |
| `std::promise<T>` | the sending end you fill by hand: `set_value()` / `set_exception()` |
| `std::packaged_task<R(Args...)>` | wraps a callable; calling it stores the result in its future |
| `std::async(policy, f, args...)` | runs `f` and returns a future for its result |

### `std::async` and launch policies

```cpp
std::future<int> square = std::async(std::launch::async, [](int x) { return x * x; }, 7);
// ... do other work ...
int result = square.get();             // waits if not finished; rethrows if f threw
```

| Policy | Behavior |
|---|---|
| `std::launch::async` | runs on a new thread (as if by `std::thread`) |
| `std::launch::deferred` | does not run until `get()`/`wait()`, then runs **on the calling thread**; never runs if nobody waits |
| default (`async \| deferred`) | the implementation chooses; you cannot rely on it running concurrently, and `wait_for()` may report `deferred` |

Pass `std::launch::async` explicitly when you need real concurrency.

### Interview point: a discarded `std::async` future blocks

```cpp
std::async(std::launch::async, [] { sleep_for(150ms); });   // statement 1
std::cout << "doing other work...\n";                       // runs 150 ms later!
```

The future returned by `std::async` is a temporary. It is destroyed at the end of statement 1, and **the destructor of the last future that refers to a `std::async` shared state waits for the task to finish**. So the code above is fully synchronous. (Futures from `std::promise` or `std::packaged_task` do not block in their destructor; only `std::async` futures do.) Since C++20 `std::async` is `[[nodiscard]]` (GCC 13 warns even in C++17 mode), so ignoring the result gives a warning; the example uses `static_cast<void>(...)` to show the blocking on purpose.

The original example "fixed" this by blocking on `std::cin.get()` at the end, but the second `std::async` had already blocked the main thread for the full sleep before "Doing other work..." was printed. The fix is to keep the future in a variable, do the other work, and then call `get()`/`wait()`.

Run it: `make run EX=cxx_async_future` ([examples/core/async_future.cpp](examples/core/async_future.cpp)) (prints how long the discarded future blocked, and shows exception propagation and `launch::deferred`).

### `std::promise`: setting the result by hand

```cpp
void async_task(std::promise<int> promise) {
  std::this_thread::sleep_for(100ms);
  promise.set_value(42);               // wakes the thread blocked in future.get()
}

std::promise<int> promise;
std::future<int> future = promise.get_future();
std::thread worker(async_task, std::move(promise));   // promise is move-only
std::cout << future.get() << '\n';      // blocks until set_value
worker.join();
```

- If the promise is destroyed without a value or exception, `get()` throws `std::future_error` with `broken_promise`.
- `get()` may be called only once on a `std::future`; a second call is undefined behavior (`valid()` becomes false). Use `shared_future` for several readers.
- A future is a one-shot result, not a stream; for repeated messages use a queue like the one in section 8.

Run it: `make run EX=cxx_promise_future` ([examples/core/promise_future.cpp](examples/core/promise_future.cpp))

### Callbacks vs futures

The original example passed a callback (`std::function<void(int)>`) into the async task. That works, but the callback then runs **on the worker thread**, so anything it touches must be thread-safe. A future returns the result to the thread that calls `get()`, and also carries exceptions, which a plain callback cannot do.

---

## 10. Why modern C++ (and not pthreads)?

| Aspect | pthreads (C) | Modern C++ |
|---|---|---|
| Resource safety | manual lock/unlock; forget an unlock and you deadlock | RAII handles it, even when exceptions are thrown |
| Type safety | `pthread_cond_wait` takes any `pthread_mutex_t*` | `condition_variable::wait` only takes `std::unique_lock<std::mutex>`; the lock type is checked at compile time (waiting on one cv with different mutexes is still undefined behavior) |
| Portability | Unix-like systems; Windows needs another API or a wrapper | standard library abstraction; one codebase compiles everywhere |
| Results and errors | pass `void*` around by hand | `std::future` carries typed values and exceptions |
| Performance | native OS calls | usually a user-space fast path (for example futex) and the kernel only on contention; comparable speed, much better safety |

**Standard interview conclusion:** unless you are writing Linux kernel code, bare-metal embedded code, or wrapping an existing C API, a modern C++ project should default to the C++11/17/20 concurrency primitives.
