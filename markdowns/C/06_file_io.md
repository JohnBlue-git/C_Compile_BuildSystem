# C Notes 06: File I/O with `<stdio.h>`

## Opening and closing

`fopen(path, mode)` returns a `FILE *`, or `NULL` with `errno` set. Every successful `fopen` needs exactly one `fclose`. After `fclose`, the `FILE *` must not be used, not even for `rewind`.

| Mode | Read | Write | File must exist | Effect on open |
|---|---|---|---|---|
| `"r"` | Yes | No | Yes | Start at offset 0 |
| `"w"` | No | Yes | No | **Truncates** to 0 bytes, or creates the file |
| `"a"` | No | Yes | No | Creates if needed; every write goes to the end |
| `"r+"` | Yes | Yes | Yes | Start at offset 0 |
| `"w+"` | Yes | Yes | No | Truncates or creates |
| `"a+"` | Yes | Yes | No | Reads anywhere; writes always go to the end |

Add `b` (`"rb"`, `"wb"`) for binary data. On POSIX systems it has no effect; on Windows text mode translates `\r\n`. C11 also adds `x` (`"wx"`): fail if the file already exists.

Opening a file with `"w"` destroys its content immediately, even if nothing is written. Do not open your input file with `"w"` to "test writing".

`fclose` flushes buffered data, so a write error (for example, a full disk) can be reported only by `fclose`. Check its return value for files you write.

## Reading and writing text

```c
char line[256];
while (fgets(line, sizeof line, file) != NULL) {
    fputs(line, stdout);
}
if (ferror(file)) { /* read error, not end of file */ }
```

- `fgets` reads at most `size - 1` characters, stops after `\n`, keeps the `\n`, and always terminates the buffer. A line longer than the buffer comes back in several pieces.
- It returns `NULL` at end of file or on error. Use `feof`/`ferror` after the loop to tell them apart; do not use `while (!feof(file))` as the loop condition (it runs one extra time).
- `fputs(string, file)` writes a string without formatting and returns `EOF` on error. Unlike `puts`, it does not add `\n`.
- `fprintf(file, format, ...)` writes formatted text and returns the number of characters, or a negative value on error.
- Never use `gets` (removed in C11): it cannot limit the input size.

## Position: `fseek`, `ftell`, `rewind`

| Call | Effect |
|---|---|
| `fseek(file, offset, SEEK_SET / SEEK_CUR / SEEK_END)` | Move the position; returns 0 on success |
| `ftell(file)` | Current position as `long`, or `-1L` on error |
| `rewind(file)` | Same as `fseek(file, 0, SEEK_SET)`, and also clears the error indicator |

A common way to get a file size:

```c
FILE *file = fopen(path, "rb");
fseek(file, 0, SEEK_END);
long size = ftell(file);   /* position at the end == size in bytes */
rewind(file);              /* back to the start, if you still read the file */
```

- Use binary mode: for a text stream, the C standard says the `ftell` value is only usable by `fseek`, not a byte count (on POSIX it is the byte offset anyway).
- `long` is 32 bits on some platforms (Windows, 32-bit Linux). For files over 2 GiB, use POSIX `ftello`/`fseeko` with `off_t`, or `stat`/`fstat`, which gives the size without opening the file for reading.

## Buffering

`stdio` streams are buffered: `stdout` is line-buffered when it is a terminal and fully buffered when it is a pipe or file; `stderr` is unbuffered. Call `fflush(stream)` when output must appear now, for example before calling `fork`, before writing to the same file descriptor with `write`, or before a crash you want to debug. `setvbuf` changes the buffering mode.

Run it: `make run EX=c_file_io` (reads `sample.txt`, writes and appends to a separate `output.txt`)
