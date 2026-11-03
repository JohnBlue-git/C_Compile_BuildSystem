/* heap_overflow.c - DELIBERATE BUG. Run it with the asan preset:
 *   make run PRESET=asan EX=memory_bug_heap_overflow
 *
 * An off-by-one loop (<= instead of <) writes one element past the end
 * of a heap array. */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    (void)argv;
    size_t count = (size_t)argc + 7; /* 8; computed at run time */
    int *values = malloc(count * sizeof *values);
    if (!values) return 1;

    for (size_t i = 0; i <= count; ++i) /* BUG: i == count is out of bounds */
        values[i] = (int)i;

    printf("last value: %d\n", values[count - 1]);
    free(values);
    return 0;
}
