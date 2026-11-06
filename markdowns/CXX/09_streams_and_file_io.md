# C++ Interview Notes 09: Streams and File I/O

## 1. The stream classes

All input streams derive from `std::istream`, all output streams from `std::ostream`. `std::cin` and `std::cout` are just predefined objects of those types, so everything below works the same on the console, a file, or a string.

| Class | Header | Direction | Source / destination |
|---|---|---|---|
| `std::istream` / `std::ostream` | `<istream>` / `<ostream>` | in / out | base classes; `cin`, `cout`, `cerr` |
| `std::ifstream` | `<fstream>` | in | file |
| `std::ofstream` | `<fstream>` | out | file |
| `std::fstream` | `<fstream>` | in and out | file |
| `std::istringstream` / `std::ostringstream` / `std::stringstream` | `<sstream>` | in / out / both | `std::string` in memory |

File streams are RAII objects: the destructor flushes and closes the file, so an explicit `close()` is only needed when you want to reopen the stream or check for errors at that point ([00](00_core_fundamentals.md)).

```cpp
std::ifstream in("data.txt");          // opens for reading
if (!in) { /* could not open */ }      // same as in.fail()

std::ofstream out("output.txt");       // truncates or creates
std::ofstream log("app.log", std::ios::app);   // append
```

**Buffering**: a stream reads from or writes to an in-memory buffer, not the device directly. That means fewer slow system calls, but it also means output may not reach the file until the buffer is flushed (`std::flush`, `std::endl`, closing the stream), and unread input stays in the buffer for the next read.

## 2. Stream state flags

Every stream has three error bits and four query functions:

| Query | True when | Typical cause |
|---|---|---|
| `good()` | no bit is set | |
| `eof()` | `eofbit`: a read reached the end of input | end of file |
| `fail()` | `failbit` **or** `badbit` | a read could not produce a value (wrong format, nothing left), or open failed |
| `bad()` | `badbit`: the stream is broken | I/O error from the device |

- `if (stream)` and `if (!stream)` test `!fail()`. Every read operation returns the stream, so `while (in >> x)` means "while that read succeeded".
- Once `failbit` is set, every later read does nothing until you call `clear()`.
- Reaching the end often sets **both** `eofbit` and `failbit` (the read that hit the end also failed to produce a value).

### Anti-pattern: `while (!in.eof())`

```cpp
// data.txt: "1 2\n3 4\n5 6\n"
char value;
while (!in.eof()) {       // WRONG
    in >> value;
    std::cout << value << '\n';
}
// prints 1 2 3 4 5 6 6   <- the last value twice
```

Why: `eof()` doesn't predict the future; it becomes true only **after** a read tries to go past the end. After `'6'` is read, the next character is `'\n'`, so `eofbit` is still clear and the loop runs again. That read skips the whitespace, hits end of file, sets `eofbit` and `failbit`, and leaves `value` unchanged, so `6` is printed again. (For an `int` target, a failed read stores `0` since C++11, so you would print `0` instead.) With no trailing newline the loop would happen to work, which is why this bug survives testing.

The fix is to test the read itself:

```cpp
while (in >> value) {
    // use value: the read succeeded
}
if (in.bad())      { /* I/O error */ }
else if (!in.eof()) { /* stopped on bad format, not at end of file */ }
```

### Rewinding: `clear()` then `seekg()`

```cpp
while (in >> number) { /* ... */ }   // stream now has eofbit | failbit
in.clear();                          // reset the error bits
in.seekg(0, std::ios::beg);          // back to the start
```

Since C++11, `seekg()` clears `eofbit` by itself, but it does nothing while `failbit` is set, so `clear()` is still required.

Run it: `make run EX=cxx_stream_state` ([examples/library/stream/stream_state.cpp](examples/library/stream/stream_state.cpp))

## 3. Reading: `>>`, `get()`, `getline()`

| | `stream >> x` | `stream.get(ch)` | `stream.getline(buf, n)` | `std::getline(stream, str)` |
|---|---|---|---|---|
| Reads into | any type with `operator>>` | one `char` | `char` array | `std::string` |
| Leading whitespace | skipped | **not** skipped | not skipped | not skipped |
| Stops at | whitespace after the token | after one character | `'\n'` or `n - 1` chars | `'\n'` |
| The `'\n'` | **left in the buffer** | read like any other char | consumed, not stored | consumed, not stored |
| Size limit | for `char[N]`, bounded by `N` since C++20 (the `char*` overload was removed) | n/a | sets `failbit` if the line is longer than `n - 1` | grows as needed |
| Use for | numbers, single words | raw characters | fixed-size C buffers | full lines (preferred) |

`std::getline(stream, str, delim)` takes a custom delimiter, which is the usual way to split a string (see [07](07_stl_containers.md#5-stdstring-and-text-storage)).

Run it: `make run EX=cxx_stream_get_getline` ([examples/library/stream/get_getline.cpp](examples/library/stream/get_getline.cpp))

### Leftover input after `>>`

```cpp
int number;
char symbol;
std::cin >> number;     // user types "3<Enter>": reads 3, leaves '\n' in the buffer
std::cin.get(symbol);   // reads that '\n' immediately; the user never gets to type
```

The same happens when `std::getline` follows `>>`: it returns an empty line. Discard the rest of the line first:

```cpp
#include <limits>
std::cin >> number;
std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // up to and including '\n'
std::cin.get(symbol);
```

`ignore(n, delim)` discards up to `n` characters, stopping after `delim`. With `max()` and `'\n'`, it discards the rest of the **current line**, not everything in the buffer. Alternatives: `std::cin >> std::ws` skips leading whitespace before a `getline`.

**`std::cin.sync()` is not a fix.** For an input stream, `sync()` calls the buffer's `pubsync()`, and what that does to unread input is implementation-defined. With libstdc++ on Linux (`cin` synchronized with stdio) it ends up as `fflush(stdin)`, which for a terminal or a pipe discards nothing. It does not "discard up to the next newline". Use `ignore()`.

| | `std::cin.sync()` | `std::cin.ignore(max, '\n')` |
|---|---|---|
| What it discards | implementation-defined, often nothing | the rest of the current line |
| Portable | no | yes |
| Control | none | count and delimiter |

Run it: `make run EX=cxx_stream_input_buffer` ([examples/library/stream/input_buffer.cpp](examples/library/stream/input_buffer.cpp)) (uses `std::istringstream` in place of `std::cin` so it runs without a keyboard).

## 4. Writing: `<<` and `put()`

```cpp
std::ofstream out("output.txt");
out << "Hello, world!\n";   // formatted output
out.put('A');               // one unformatted character
out.put('\n');
```

- `'\n'` vs `std::endl`: `endl` writes `'\n'` **and flushes**. Flushing on every line is slow for files; use `'\n'` and let the destructor (or an explicit `std::flush`) flush.
- `std::cerr` is unbuffered; `std::clog` is buffered.
- Check `if (!out)` after opening and, for important data, after the last write or `close()`.

Run it: `make run EX=cxx_stream_ofstream_put` ([examples/library/stream/ofstream_put.cpp](examples/library/stream/ofstream_put.cpp)) (writes `output.txt` into the test's working directory).

## 5. Interview points

- **"Why is `while (!in.eof())` wrong?"** Because `eof()` reports the past, not the future; the last read fails and the loop body still runs. Loop on the read: `while (in >> x)` or `while (std::getline(in, line))`.
- **"Why does `getline` return an empty string after `cin >> n`?"** `>>` leaves the `'\n'`; `getline` reads up to it. Call `ignore()` first.
- **`fail()` vs `bad()`**: `fail()` is usually recoverable (bad format: `clear()` and skip the input); `bad()` means the stream itself is broken.
- **RAII**: `ifstream`/`ofstream` close in their destructors; no `try/finally` needed.
- **C stdio vs iostreams**: iostreams are type-safe and extensible (`operator<<` for your own types). `std::ios::sync_with_stdio(false)` (plus `cin.tie(nullptr)`) speeds up console I/O when you don't mix in `printf`/`scanf`. C file I/O is in [C](../C/README.md).
