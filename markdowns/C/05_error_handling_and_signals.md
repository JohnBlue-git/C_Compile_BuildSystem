# C Notes 05: Error Handling and Signals

C has no exceptions. Errors are reported through return values and `errno`. Hardware exceptions (a processor fault such as an invalid memory access or an integer division by zero) transfer control to the kernel, which on Unix-like systems usually turns them into a **signal** sent to the process.

## Segmentation fault

A segmentation fault (`SIGSEGV`) happens when the program accesses memory it is not allowed to access: an unmapped address, or a mapped page with the wrong permission (for example, a write to a read-only page).

Common causes:

- Dereferencing `NULL`, an uninitialized pointer, or a dangling pointer (after `free` or after the pointed-to local goes out of scope).
- Writing to read-only memory: a string literal, or a `const` object that the compiler placed in `.rodata`.
- Accessing far outside an array. A small overrun often does **not** crash, because the address is still mapped; it silently corrupts other data. That is why "no crash" proves nothing.
- Stack overflow, for example from unbounded recursion or a very large local array.

Prevention:

- Initialize every pointer (to a valid address or `NULL`).
- Check the result of `malloc`/`calloc`/`fopen` and similar APIs before use.
- After `free(p)`, set `p = NULL` if `p` stays in scope. `free(NULL)` is a no-op, so no check is needed before `free`.
- Prefer stack objects and pass their address (`f(&x)`) instead of allocating on the heap when the lifetime allows it.
- Pass array lengths explicitly: after an array is passed to a function, `sizeof` gives the pointer size ([00](00_fundamentals.md#pointers-and-arrays)).
- Build with `-fsanitize=address,undefined` during development.

## Ending the program: `return`, `exit`, `_Exit`, `abort`

| Call | `atexit` handlers | Flush/close `stdio` streams | Notes |
|---|---|---|---|
| `return n;` from `main` | Yes | Yes | Same as `exit(n)` |
| `exit(n)` | Yes | Yes | Normal termination with status `n` |
| `_Exit(n)` (C99) / `_exit(n)` (POSIX) | No | No | Immediate; `_exit` is async-signal-safe |
| `abort()` | No | Not guaranteed (glibc does not flush) | Raises `SIGABRT`; usually produces a core dump |

None of them run cleanup for heap memory you did not free; the operating system reclaims the process memory. Call `exit`/`abort` only for unrecoverable errors. A library should return an error code and let the caller decide.

## `assert`

`assert(expression)` checks a condition that must always be true if the program is correct (a programmer error, not a runtime error such as a missing file). When it fails, it prints the expression, file, and line, then calls `abort()`.

- Define `NDEBUG` to remove all asserts: `-DNDEBUG` on the command line (CMake Release builds add it), or `#define NDEBUG` **before** `#include <assert.h>`.
- Because asserts can disappear, never put work with side effects inside them: `assert(fclose(f) == 0)` would skip the `fclose` in release builds.
- The run-time cost of an assert is usually just a comparison; the reason to disable it is that it is a debugging tool, not error handling.
- `static_assert(condition, "message")` (C11, from `<assert.h>`) checks at compile time, for example `static_assert(sizeof(int) == 4, "need 32-bit int");`.

## Return codes, `errno`, and goto-cleanup

Functions report failure with a return value (`-1`, `NULL`, `EOF`). Many standard and POSIX functions also set `errno`; read it immediately, because the next library call can change it. `perror("context")` prints `context: <strerror(errno)>`.

A function that acquires several resources often uses one `cleanup` label, so every failure path releases what was acquired, in reverse order:

```c
int process(const char *path) {
    int result = -1;
    char *buffer = NULL;
    FILE *file = NULL;

    buffer = malloc(256);
    if (buffer == NULL) goto cleanup;

    file = fopen(path, "r");
    if (file == NULL) { perror(path); goto cleanup; }

    /* ... work ... */
    result = 0;

cleanup:
    if (file != NULL) fclose(file);
    free(buffer);
    return result;
}
```

This is the standard C replacement for destructors/RAII (see the [C++ notes](../CXX/README.md)). The Linux kernel uses the same pattern.

Run it: `make run EX=c_error_cleanup` ([examples/error_cleanup.c](examples/error_cleanup.c))

## Signals

A signal is an asynchronous notification sent to a process: by the kernel (fault, timer, child exit), by another process (`kill`), by the terminal (Ctrl+C), or by the process itself (`raise`, `abort`). Each signal has a default action: terminate, terminate with core dump, ignore, stop, or continue. A process can replace the action with:

- a handler function,
- `SIG_IGN` to ignore the signal,
- `SIG_DFL` to restore the default action.

`signal()` returns `SIG_ERR` on failure. `SIGKILL` and `SIGSTOP` cannot be caught, blocked, or ignored.

### Common Linux signals

Numbers are for Linux on x86 and ARM. Some other architectures use different numbers, so always use the names.

| Signal | No. | Default | Typical cause |
|---|---|---|---|
| `SIGHUP` | 1 | Terminate | Controlling terminal closed; daemons often use it as "reload config" |
| `SIGINT` | 2 | Terminate | Ctrl+C |
| `SIGQUIT` | 3 | Core dump | Ctrl+\\ |
| `SIGILL` | 4 | Core dump | Illegal instruction |
| `SIGABRT` | 6 | Core dump | `abort()`, failed `assert` |
| `SIGBUS` | 7 | Core dump | Bus error, for example a misaligned access on some CPUs or access to a truncated `mmap` file |
| `SIGFPE` | 8 | Core dump | Integer division by zero, `INT_MIN / -1` (floating-point division by zero gives `inf` and does not trap by default) |
| `SIGKILL` | 9 | Terminate | `kill -9`; cannot be caught, blocked, or ignored |
| `SIGUSR1` / `SIGUSR2` | 10 / 12 | Terminate | Application-defined |
| `SIGSEGV` | 11 | Core dump | Invalid memory access |
| `SIGPIPE` | 13 | Terminate | Write to a pipe or socket with no reader (servers often set it to `SIG_IGN` and handle `EPIPE`) |
| `SIGALRM` | 14 | Terminate | Timer from `alarm()` |
| `SIGTERM` | 15 | Terminate | Polite termination request (default for `kill`); can be caught |
| `SIGCHLD` | 17 | Ignore | A child process stopped or ended |
| `SIGCONT` | 18 | Continue | Resume a stopped process |
| `SIGSTOP` | 19 | Stop | Stop the process; cannot be caught, blocked, or ignored |
| `SIGTSTP` | 20 | Stop | Ctrl+Z; can be caught |

Real-time signals run from `SIGRTMIN` to `SIGRTMAX` (kernel range 32 to 64; glibc reserves the first two for threads, so `SIGRTMIN` is usually 34). Always write them as `SIGRTMIN + n`, never as fixed numbers. See `man 7 signal` for the full list.

### Writing a safe handler

A handler can interrupt the program at any instruction, including inside `malloc` or `printf` while they hold a lock. So a handler may only call **async-signal-safe** functions (`man 7 signal-safety`): for example `write`, `_exit`, `signal`, `sem_post`, `kill`. `printf`, `malloc`, `free`, and `exit` are **not** safe.

The usual design: the handler only sets a flag, and the main program does the real work.

```c
static volatile sig_atomic_t stop = 0;

static void on_signal(int signum) {
    stop = signum;              /* volatile sig_atomic_t: the only type C guarantees here */
}

struct sigaction action = {0};
action.sa_handler = on_signal;
sigemptyset(&action.sa_mask);
sigaction(SIGTERM, &action, NULL);
```

- Prefer `sigaction` to `signal`. The behavior of `signal` differs between systems (System V resets the handler to `SIG_DFL` after each delivery; BSD and glibc do not), and `sigaction` gives control over the blocked mask and flags such as `SA_RESTART`.
- **Do not return from a handler for a hardware fault** (`SIGSEGV`, `SIGFPE`, `SIGILL`, `SIGBUS` caused by the instruction itself): the faulting instruction runs again, which usually loops forever, and the behavior is undefined. Log with `write` and call `_exit`, or `siglongjmp` out.
- Note that integer division by zero is already UB in C; do not rely on getting `SIGFPE`. Check the divisor first.

Run it: `make run EX=c_signal_handler` ([examples/signal_handler.c](examples/signal_handler.c)) (sigaction, a flag, `SIG_IGN`, `SIG_DFL`)

### Timeout pattern

Wait until a timer (`alarm`, `SIGALRM`) or a termination request (`SIGINT`, `SIGTERM`) arrives. Checking a flag and then calling `pause()` has a race: the signal can arrive between the check and the `pause`, and the program sleeps forever. Block the signals first, then use `sigsuspend`, which atomically unblocks them and sleeps:

```c
sigprocmask(SIG_BLOCK, &blocked, &previous);
alarm(1);
while (stop == 0) {
    sigsuspend(&previous);      /* returns after a handler has run */
}
sigprocmask(SIG_SETMASK, &previous, NULL);
```

A semaphore also works, because `sem_post` is async-signal-safe: the handler calls `sem_post`, and the main thread calls `sem_wait` (retry when it fails with `EINTR`). Initialize the semaphore before installing the handlers. For new Linux code, `signalfd` or a self-pipe lets an event loop handle signals as normal file descriptor events.

Run it: `make run EX=c_signal_timeout` ([examples/signal_timeout.c](examples/signal_timeout.c))

Reference: [sysprog: C stream I/O and signals](https://hackmd.io/@sysprog/c-stream-io#signal), `man 7 signal`, `man 7 signal-safety`.
