# Stack, Heap, and Object Lifetime

Stack and heap are not concepts that belong to C or C++. They describe common ways to use storage. Where things are really placed, which way the stack grows, and how the allocator works depend on the ABI, the operating system, the compiler, and the runtime. Do not treat a simplified diagram as a language guarantee.

## Common memory regions

```text
High addresses
┌────────────────────────────┐
│ Stack                      │  function frames, local variables; invalid after the scope ends
├────────────────────────────┤
│ mmap / shared libraries    │  dynamically loaded libraries and memory mappings
├────────────────────────────┤
│ Heap                       │  dynamic storage managed by malloc/new/allocators
├────────────────────────────┤
│ BSS / Data                 │  globals and other variables with static storage duration
├────────────────────────────┤
│ Text / read-only data      │  machine code, string literals, some constants
└────────────────────────────┘
Low addresses
```

This is a conceptual model of a typical process layout. Not every system uses the same order or contiguous regions.

> Run it: `make run EX=memory_storage_duration` ([examples/storage_duration.c](examples/storage_duration.c)). It sorts the addresses of a local, a `malloc` block, two globals, a string literal, and `main`:

```text
Highest address first:
  0x7ffe2bb6d938  stack     (automatic: on_stack)
  0x641a9bb812a0  heap      (dynamic: malloc)
  0x641a60395018  .bss      (static: zero_global)
  0x641a60395010  .data     (static: initialized_global)
  0x641a603930aa  .rodata   (string literal)
  0x641a6039226d  .text     (code: main)
count_calls(): 11 21 31
```

The addresses change on every run (ASLR), but on x86_64 Linux the order matches the diagram. Shared libraries are mapped near the stack (`0x7f...`), far above the heap. The program's own `.text`/`.data`/`.bss` sit together, and the heap starts above them. Under the `asan` preset, the order changes. ASan replaces `malloc` with its own allocator and can move locals to a "fake stack", which is one more reason not to rely on the picture. The last line shows lifetime: `count_calls()` returns `calls * 10 + local`. The `static` counter keeps growing (1, 2, 3), and the automatic `local` is a new object on every call (always 1).

## Storage duration and lifetime

| Kind | Typical example | Lifetime |
|---|---|---|
| automatic | `int value` inside a function | Created when the block is entered; ends when the block ends |
| static | File-scope variable, `static` local variable | The whole program run |
| thread | `thread_local` (C++11, C23) / `_Thread_local` (C11) | The life of its thread |
| dynamic | Storage from `malloc`, `new`, an allocator | Until it is freed explicitly or by its owner |

"Storage" and "object lifetime" are not the same thing. Raw memory can exist while the C++ object in it has not been constructed yet, or has already been destroyed. That is why placement `new` and explicit destructor calls need care.

> Run it: `make run EX=memory_object_lifetime` ([examples/object_lifetime.cpp](examples/object_lifetime.cpp)). A `Tracer` type prints when it is constructed and destroyed:

```text
  construct global (static storage)
main starts
  construct automatic
  ...leaving the block
  destroy   automatic
  construct new/delete
  destroy   new/delete
  construct unique_ptr
  destroy   unique_ptr
  construct placement new
  destroy   placement new
  construct function-local static
main returns
  destroy   function-local static
  destroy   global (static storage)
```

- The global is constructed **before** `main` and destroyed **after** it.
- A function-local `static` is constructed on the first call only (and thread-safely since C++11). Static objects are destroyed in the reverse order of construction.
- `new`/`delete`: the object lives until someone calls `delete`. `unique_ptr` does that call when its own scope ends.
- Placement `new` constructs an object in a buffer that already exists. The program must call the destructor by hand, and the buffer itself outlives the object.

## Allocation in C and C++

| Need | C | C++ |
|---|---|---|
| Raw bytes | `malloc` / `calloc` / `realloc` | An allocator or `::operator new` |
| Create an object | Initialize the struct by hand | `new T(...)` calls the constructor |
| Destroy an object | `free` | `delete` calls the destructor, then frees |
| Array | `malloc(count * size)` | `new T[count]` with `delete[]` |
| Recommended owner | Document who must call `free` | `std::unique_ptr`, `std::vector`, an RAII wrapper |

`malloc`/`free` do not call C++ constructors or destructors, and you must not mix `new`/`delete` with `malloc`/`free`. In C++ application code, use RAII members or smart pointers so that cleanup is tied to a scope. See [C++ smart pointers](../CXX/01_smart_pointers.md).

## Function calls, callbacks, and the stack

### What is on the stack during a call?

A function's machine code is usually in the text (code) segment. It is not copied to the stack for each call. For a call, the runtime/ABI creates or uses a call frame. The frame holds the return address, saved registers, some arguments, local variables, and temporaries:

```text
Text / Code
┌────────────────────────────┐
│ process_data() machine code│
│ on_event() machine code    │
└──────────────┬─────────────┘
               │ call / indirect call
               ▼
Thread stack (conceptual)
┌────────────────────────────┐
│ process_data frame         │
│  return address            │
│  parameters / locals       │
├────────────────────────────┤
│ on_event frame             │  created while the callback runs
│  callback parameters       │
│  local variables           │
└────────────────────────────┘
```

From a storage point of view, a direct call and a callback through a function pointer are the same. There is no rule that "callbacks live on the heap". Both create call state. The difference is that a callback is usually an indirect jump through a function pointer:

```c
typedef void (*callback_fn)(int value, void *context);

void run_now(callback_fn callback, void *context) {
    int local_value = 42;           /* automatic storage in this frame */
    callback(local_value, context); /* the callback frame goes on top of run_now */
}
```

Keep these apart:

- The callback's machine code is usually in the text (code) segment.
- A variable that holds the callback's function pointer can be on the stack, in static storage, or on the heap. It points to code; the code does not move to that region.
- Callback arguments can be in registers or on the stack, depending on the ABI, the optimization level, and the argument types. "A call always pushes its arguments on the stack" is not a C/C++ guarantee (the x86_64 System V ABI passes the first six integer arguments in registers).
- Local variables in a callback usually have automatic storage duration, but the compiler can keep them in registers or remove them completely.

### Synchronous callbacks: stack context is safe

If the callback runs before the called function returns, passing the address of a local variable is safe, because the callback cannot escape that scope:

```c
void visit_now(callback_fn callback) {
    int context_value = 7;
    callback(42, &context_value);  /* the scope ends only after the callback returns */
}
```

But if the callback stores the pointer, nobody may use it after the function returns:

```text
visit_now() stack frame
┌──────────────────────┐
│ context_value = 7    │──┐ valid while the callback runs
└──────────────────────┘  │
                          ▼
                    callback stores the address
                          │
                    visit_now() returns
                          │
                    the frame is reused
                          ▼
                    dangling pointer
```

### Asynchronous callbacks: the context must escape to a longer lifetime

A callback registered with an event loop, a thread, a timer, or an interrupt-like framework usually runs **after** the registering function returns. Then you must not pass the address of a local variable:

```c
void start_timer_bad(void) {
    struct TimerContext context = {0};
    register_timer(on_timeout, &context);
}   /* context is dead, but the timer may call on_timeout later */
```

The safe options give the context a lifetime that covers the callback's lifetime:

```text
The context must live at least as long as the callback can run

stack local       [------scope------]                       not for async callbacks
heap owner        [-------------task lifetime-------------]
static storage    [-------------whole program-------------------]
shared owner      [------until the last task/owner ends------]
```

C APIs usually pass `void *context` together with a destroy callback. C++ APIs can use `std::shared_ptr`, a move-only task, or an RAII registration token, so the unregister time and the context ownership are clear.

> Run it: `make run EX=memory_callback_lifetime` ([examples/callback_lifetime.c](examples/callback_lifetime.c)). A synchronous visit uses a local `sum`. A tiny task queue then runs two asynchronous timers *after* `start_timers()` returned. One context is on the heap (the queue owns it and calls `free` as its destroy function), and one is in static storage:

```text
synchronous: sum = 10
asynchronous: start_timers() returned, now run the queue
  task 0: timer 'heap' fired
  task 1: timer 'static' fired
```

The broken version with a local context is the bug demo `memory_bug_stack_use_after_return` (see [Sanitizer demos](#sanitizer-demos-deliberate-bugs)).

### Lambdas, member callbacks, and `std::function`

```cpp
std::string name = "worker";
auto safe = [name] { use(name); };       // capture by value: the closure owns its own name
auto unsafe = [&name] { use(name); };    // capture by reference: must not outlive name
```

If the callback is stored, a capture by reference can dangle. A capture by value does not help if what you copy is a raw pointer: the object it points to may be gone. A member-function callback also stores `this`, so the object must outlive the callback, or the callback must use weak ownership (`std::weak_ptr`) to check that the object still exists. See [C++ lambdas and callbacks](../CXX/04_lambdas_and_callbacks.md).

`std::function` may store a small callable inside itself (small-object optimization) and may allocate dynamic storage for a large one. That is an implementation detail. Do not rely on it being on the stack or on the heap. What you must guarantee is the ownership and lifetime of what the callable captures.

## Advanced cases

### Every thread has its own stack

The main thread and each worker thread usually have separate stacks. A callback that runs on a worker thread uses the worker's call stack, not the stack of the thread that registered it. Before you pass a pointer to another thread, make sure the object's lifetime, the synchronization, and the absence of data races are all correct:

```text
main thread stack                 worker thread stack
┌──────────────────┐              ┌──────────────────┐
│ register task    │  enqueue     │ callback frame   │
│ local variables  │ ───────────> │ task locals      │
└──────────────────┘              └──────────────────┘
```

A `thread_local` variable has one instance per thread. Its lifetime usually runs from the start to the end of that thread. It is not a static object that all threads share. See [C++ concurrency](../CXX/02_concurrency.md).

### A coroutine is not a normal stack frame

When a C++ coroutine suspends, the coroutine frame must keep its locals, the promise, and the resume state during the suspension. The coroutine machinery usually allocates this frame in dynamic storage. A custom allocator or compiler elision can change that. Do not treat locals of a suspended coroutine as part of the caller's stack frame.

### Recursion, stack overflow, and optimization

Each recursive call usually needs a new call frame, so a very deep recursion overflows the stack (the default main-thread stack on Linux is 8 MiB; see `ulimit -s`). Tail-call optimization, inlining, register allocation, and named return value optimization can remove frames or move data. Those are compiler/ABI results. They never extend the language lifetime of a local variable.

### Stack/heap is not a C++ semantic category

```cpp
int compute() {
    int value = 42;
    return value;       // the compiler may return it in a register, without ever using the stack
}
```

C++ guarantees object lifetime, storage duration, ownership, and observable behavior. It does not guarantee that every local object is on the stack or that every dynamic object is on a traditional heap. When you analyze a bug, first ask: who owns it, when does it end, does it escape, is access synchronized? Only then talk about memory regions.

## Common mistakes

```text
return &local;                 ──> dangling pointer: local has left its scope
free(pointer); free(pointer);  ──> double free
new T[n]; delete pointer;      ──> new[] must be paired with delete[]
vector.push_back(...);         ──> may invalidate existing references/iterators
```

- A stack overflow is usually caused by deep recursion or a large local array.
- A heap leak is an allocation without an owner or without a cleanup path.
- A use-after-free uses a pointer, reference, or iterator after the object's lifetime ended.
- `sizeof(pointer)` is the size of the pointer itself, not of the allocation it points to.

## Sanitizer demos (deliberate bugs)

The bugs above are often silent in a normal build. The program prints garbage or nothing at all, and it may crash much later or never. AddressSanitizer (ASan) instruments every memory access and stops at the first invalid one. The `asan` preset compiles everything with `-fsanitize=address,undefined -fno-omit-frame-pointer` (see [Modern CMake, section 3](../Compile/02_modern_cmake.md#3-compile-options-and-sanitizers)).

These four targets are built with `NO_TEST`, so `ctest` does not run them. Run each one by hand with the `asan` preset. ASan exits with a non-zero status, so `make` also reports an error at the end. That is expected.

| Target | Bug | Run |
|---|---|---|
| `memory_bug_use_after_free` | A second, non-owning pointer reads a freed buffer | `make run PRESET=asan EX=memory_bug_use_after_free` |
| `memory_bug_stack_use_after_return` | An async callback uses a local whose function returned | `make run PRESET=asan EX=memory_bug_stack_use_after_return` |
| `memory_bug_heap_overflow` | An off-by-one loop (`<=`) writes past a heap array | `make run PRESET=asan EX=memory_bug_heap_overflow` |
| `memory_bug_leak` | The caller never frees memory returned by a function | `make run PRESET=asan EX=memory_bug_leak` |

The key lines of each report are below (from the Docker toolchain, gcc 13.3; addresses and process IDs change; `...` marks lines left out). Read a report in this order: the **error type** on the first line, the **access** (READ/WRITE, size) and the stack where it happened, then **where the memory came from** (allocated/freed here, or the stack frame that owned it).

**Use after free** ([bugs/use_after_free.c](examples/bugs/use_after_free.c)):

```text
==30==ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010 at pc ...
READ of size 2 at 0x502000000010 thread T0
    #0 ... in printf_common ...
    ...
    #3 ... in main /work/Memory/examples/bugs/use_after_free.c:27
0x502000000010 is located 0 bytes inside of 16-byte region [0x502000000010,0x502000000020)
freed by thread T0 here:
    #0 ... in free ...
    #1 ... in main /work/Memory/examples/bugs/use_after_free.c:25
previously allocated by thread T0 here:
    ...
SUMMARY: AddressSanitizer: heap-use-after-free ... in printf_common
```

The read happens inside `printf` (ASan intercepts it), called from line 27. ASan shows all three stacks: the bad access, the `free`, and the `malloc`. Without ASan, the debug build printed `cached name: ` followed by garbage bytes and exited with status 0.

**Stack use after return** ([bugs/stack_use_after_return.c](examples/bugs/stack_use_after_return.c)):

```text
==33==ERROR: AddressSanitizer: stack-use-after-return on address 0x7cdb4e700020 at pc ...
READ of size 4 at 0x7cdb4e700020 thread T0
    #0 ... in on_timeout /work/Memory/examples/bugs/stack_use_after_return.c:26
    #1 ... in main /work/Memory/examples/bugs/stack_use_after_return.c:37
Address 0x7cdb4e700020 is located in stack of thread T0 at offset 32 in frame
    #0 ... in start_timer_bad /work/Memory/examples/bugs/stack_use_after_return.c:30
  This frame has 1 object(s):
    [32, 36) 'context' (line 31) <== Memory access at offset 32 is inside this variable
SUMMARY: AddressSanitizer: stack-use-after-return /work/Memory/examples/bugs/stack_use_after_return.c:26 in on_timeout
```

ASan names the dead variable (`context`, line 31) and the function whose frame owned it (`start_timer_bad`). To find this bug, ASan gives each frame a separate "fake stack" slot that it keeps poisoned after the function returns. GCC 13's runtime enables this check by default (`detect_stack_use_after_return=1`). With `ASAN_OPTIONS=detect_stack_use_after_return=0`, the same binary prints `timer fired 1 time(s)` and exits with 0, which is also what the non-ASan build does. Some other toolchains turn the check off by default.

**Heap buffer overflow** ([bugs/heap_overflow.c](examples/bugs/heap_overflow.c)):

```text
==36==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x503000000060 at pc ...
WRITE of size 4 at 0x503000000060 thread T0
    #0 ... in main /work/Memory/examples/bugs/heap_overflow.c:16
0x503000000060 is located 0 bytes after 32-byte region [0x503000000040,0x503000000060)
allocated by thread T0 here:
    #0 ... in malloc ...
    #1 ... in main /work/Memory/examples/bugs/heap_overflow.c:12
SUMMARY: AddressSanitizer: heap-buffer-overflow /work/Memory/examples/bugs/heap_overflow.c:16 in main
```

"0 bytes after 32-byte region" means the write is the first `int` past an 8-`int` array: a classic off-by-one. Without ASan, the write lands in the allocator's padding, and the program printed `last value: 7` normally.

**Leak** ([bugs/leak.c](examples/bugs/leak.c)), reported by LeakSanitizer when the program exits:

```text
==19==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 37 byte(s) in 3 object(s) allocated from:
    #0 ... in malloc ...
    #1 ... in make_greeting /work/Memory/examples/bugs/leak.c:11
    #2 ... in main /work/Memory/examples/bugs/leak.c:21

SUMMARY: AddressSanitizer: 37 byte(s) leaked in 3 allocation(s).
```

A "direct" leak is a block that no pointer refers to any more. An "indirect" leak is a block reachable only from other leaked blocks. 37 bytes = `"Hello, alice"` (13) + `"Hello, bob"` (11) + `"Hello, carol"` (13), with the terminating `'\0'` included. LeakSanitizer uses ptrace-like process inspection, which is why the `Makefile` runs Docker with `--cap-add=SYS_PTRACE`.

Notes on the demo sources:

- At `-O2` (the `release` preset), GCC finds two of these bugs at **compile** time (`-Wuse-after-free`, `-Wdangling-pointer`) after inlining. The demos store the aliasing pointer in a `volatile` variable so that the optimizer cannot follow it, and the bug reaches run time. In real code, take those warnings seriously.
- Valgrind (`valgrind ./memory_bug_leak` on a normal debug build) finds the heap bugs and the leak without recompiling, but it is much slower, and it cannot see stack-use-after-return or overflows within the stack.

## Containers and storage

```text
std::array<int, 3>       the object holds [a][b][c] directly
std::vector<int>         the object holds ptr/size/capacity; the elements are usually in dynamic storage
std::list<int>           each element is usually a separate node, linked by pointers
std::string              short strings may live inside the object (SSO); long strings use dynamic storage
```

Do not judge performance only by "it looks like stack" or "it looks like heap". Also consider the number of allocations, cache locality, object lifetime, iterator stability, and ownership. For choosing and comparing containers, see [C++ STL containers](../CXX/07_stl_containers.md).
