# C Notes 00: Language and Memory Basics

## Compiling and linking

The preprocess, compile, assemble, and link stages of C, and how `gcc`, libraries, and CMake map onto them, are covered in the [Compile section](../Compile/00_compile_and_linkage.md). This note covers only the C language itself.

## Pointers and arrays

An array is not a pointer, but in most expressions an array decays to a pointer to its first element. That is why a function that takes an array also needs the length as a separate argument:

```c
void sum(const int values[], size_t count) {
    for (size_t i = 0; i < count; ++i) {
        /* in a parameter list, values is really const int * */
    }
}
```

Inside that function, `sizeof(values)` gives the size of a pointer, not the number of elements. To keep the length, use `sizeof(array) / sizeof(array[0])` in the scope where the array is declared, or pass an explicit `count`.

### Dynamic 2D arrays

A common interview pattern is a 2D array built as an array of row pointers (`int **`): allocate the row table, then each row, and free in the reverse order.

```c
int **rows = calloc(n_rows, sizeof *rows);       /* row pointers start as NULL */
for (int i = 0; i < n_rows; i++) {
    rows[i] = malloc(n_cols * sizeof *rows[i]);  /* contents are indeterminate */
    if (rows[i] == NULL) { /* free rows[0..i) and rows, then fail */ }
}
/* ... rows[i][j] ... */
for (int i = 0; i < n_rows; i++) free(rows[i]);
free(rows);
```

- `malloc` leaves memory uninitialized; `calloc(count, size)` zeroes it (and checks `count * size` for overflow).
- If one row allocation fails, free the rows that already succeeded. Returning early without that is a leak.
- Passing `int **` lets the function modify the caller's elements, but the function still gets a *copy* of the pointer. See [02](02_functions_callbacks_and_arguments.md).
- When the column count is fixed, one contiguous block (`int (*grid)[COLS] = malloc(rows * sizeof *grid);`) needs only one `malloc`/`free` and is more cache-friendly.

Run it: `make run EX=c_pointer_2d` ([examples/pointer_2d.c](examples/pointer_2d.c))

## Strings and lifetime

A C string is a sequence of `char` that ends with `\0`. The destination buffer must have room for the terminator, and string literals must not be modified:

```c
char name[8] = "John";        /* modifiable array */
const char *label = "resume"; /* string literal: read-only */
```

`malloc` returns uninitialized memory; `calloc` zeroes it. Every successful allocation needs a clear owner that calls `free`. `free(p)` releases the heap block that `p` points to, not the pointer variable itself; afterwards `p` is dangling, so set it to `NULL` if it stays in scope. `free(NULL)` is a no-op, so a NULL check before `free` is not needed. When `realloc` fails it returns `NULL` and the original block is still valid, so do not assign the result directly to the only pointer that owns the block.

## `switch`

```c
switch (choice) {
case 1:
    puts("one");
    break;
case 3:
case 4:              /* empty case falls through: 3 and 4 share a body */
    puts("three or four");
    break;
default:
    puts("invalid");
    break;
}
```

- The controlling expression must have an integer type (including `char` and `enum`); case labels must be integer constant expressions.
- Without `break`, execution falls through into the next case. Mark intended fall-through after non-empty code with a comment (or `__attribute__((fallthrough))` in GCC/Clang, `[[fallthrough]]` in C23); `-Wextra` enables `-Wimplicit-fallthrough`.

Run it: `make run EX=c_switch` ([examples/switch.c](examples/switch.c))

## Common undefined behavior (UB) questions

- Using a pointer to a local variable after the function returns (returning the pointer is allowed; dereferencing it is UB).
- Reading an uninitialized automatic variable.
- Out-of-bounds array access, use-after-free, double free.
- Signed integer overflow, and integer division by zero.
- Dereferencing a pointer with the wrong alignment or an incompatible type (strict aliasing).
- Modifying a `const` object or a string literal. See [04](04_qualifiers_and_preprocessor.md).

Compile with at least `-Wall -Wextra -Wpedantic`, and test with AddressSanitizer/UndefinedBehaviorSanitizer to find memory errors and UB.
