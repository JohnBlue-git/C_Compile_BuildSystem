# C Notes 03: Strings, Memory, and Copy APIs

## Common copy and string APIs

| API | Use for | Key points |
|---|---|---|
| `memset(destination, value, count)` | Fill bytes | Sets `count` bytes to `(unsigned char)value`; fine for `0`, but `memset(ints, 1, ...)` does not make each `int` equal to 1 |
| `memcpy(destination, source, count)` | Any bytes | Copies exactly `count` bytes; source and destination must not overlap; does not know about `\0` |
| `memmove(destination, source, count)` | Any bytes | Handles overlapping regions; you still must check the capacity |
| `strcpy(destination, source)` | `\0`-terminated strings | Destination must hold `strlen(source) + 1` bytes, otherwise buffer overflow |
| `strncpy(destination, source, count)` | Fixed-size string fields | If `strlen(source) >= count`, no `\0` is written; if shorter, pads the rest with `\0`. Terminate by hand |
| `snprintf(destination, capacity, format, ...)` | Bounded formatted strings | With `capacity > 0` the result is always `\0`-terminated; the return value shows truncation |

```c
char destination[16];
int written = snprintf(destination, sizeof destination, "%s", source);
if (written < 0 || (size_t)written >= sizeof destination) {
    /* encoding error, or the output was truncated */
}
```

For every copy function, the rule is the same: the destination capacity must be at least the number of bytes written (for strings, including the `\0`). Writing past the end may appear to work, but it corrupts other memory and is UB.

- `memcpy` is for non-overlapping raw memory. When the regions can overlap (for example, shifting data inside one buffer), use `memmove`. `memmove` is not "safer" in general; it only handles overlap.
- `memcpy` works for any object: `memcpy(dst_ints, src_ints, sizeof src_ints);`. To copy a string with its terminator, copy `strlen(s) + 1` bytes (equal to `sizeof s` when `s` is an array initialized from a literal).
- Do not use `sizeof pointer` as the capacity of the buffer it points to. APIs should take the capacity explicitly.
- Avoid variable-length arrays (`char buffer[n];`) for copies: they are optional since C11 and an unchecked `n` can overflow the stack.

Run it: `make run EX=c_copy`

## Ownership with allocation APIs

```c
int *value = malloc(sizeof *value);
if (value == NULL) {
    return -1;
}
*value = 42;
free(value);
value = NULL;
```

`sizeof *value` is less error-prone than `sizeof(int)` when the type changes. Never call `free` on a stack address, and never dereference the pointer after `free`. More on storage duration and lifetime: [Memory](../Memory/00_stack_heap_and_object_lifetime.md).

## API design checklist

- Does the function state who allocates, who frees, and whether the return value can be `NULL`?
- Does a buffer API also take the capacity, and does it check for integer overflow?
- Does every failure path release the resources already acquired? (See the goto-cleanup pattern in [05](05_error_handling_and_signals.md).)
- Is `const` used to say "this function does not modify the input"?
