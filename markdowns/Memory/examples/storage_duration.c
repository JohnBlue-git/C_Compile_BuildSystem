/* storage_duration.c - where objects with each storage duration live,
 * and how long they keep their values.
 *
 * The addresses change on every run (ASLR). The order is typical for
 * x86_64 Linux, but no language standard promises it. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int initialized_global = 1; /* static storage, .data */
int zero_global;            /* static storage, .bss  */

struct Region {
    const char *name;
    uintptr_t address;
};

static int compare_desc(const void *a, const void *b) {
    uintptr_t x = ((const struct Region *)a)->address;
    uintptr_t y = ((const struct Region *)b)->address;
    return (x < y) - (x > y);
}

static int count_calls(void) {
    static int calls = 0; /* static: initialized once, keeps its value */
    int local = 0;        /* automatic: a new object on every call */
    ++local;
    return ++calls * 10 + local;
}

int main(void) {
    int on_stack = 2;
    int *on_heap = malloc(sizeof *on_heap);
    if (!on_heap) return 1;

    struct Region regions[] = {
        {"stack     (automatic: on_stack)", (uintptr_t)&on_stack},
        {"heap      (dynamic: malloc)", (uintptr_t)on_heap},
        {".bss      (static: zero_global)", (uintptr_t)&zero_global},
        {".data     (static: initialized_global)", (uintptr_t)&initialized_global},
        {".rodata   (string literal)", (uintptr_t)"literal"},
        {".text     (code: main)", (uintptr_t)main},
    };
    size_t count = sizeof regions / sizeof regions[0];
    qsort(regions, count, sizeof regions[0], compare_desc);

    puts("Highest address first:");
    for (size_t i = 0; i < count; ++i)
        printf("  %#14jx  %s\n", (uintmax_t)regions[i].address, regions[i].name);

    /* calls*10 + local: the static counter grows, the local is always 1. */
    int first = count_calls();
    int second = count_calls();
    int third = count_calls();
    printf("count_calls(): %d %d %d\n", first, second, third);

    free(on_heap);
    return (first == 11 && second == 21 && third == 31) ? 0 : 1;
}
