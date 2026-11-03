/* use_after_free.c - DELIBERATE BUG. Run it with the asan preset:
 *   make run PRESET=asan EX=memory_bug_use_after_free
 *
 * The owner frees the buffer, but a second, non-owning pointer still
 * refers to it and is used later. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Non-owning alias that outlives the buffer. volatile hides it from the
 * optimizer; at -O2 GCC would otherwise see the bug and warn
 * (-Wuse-after-free) at compile time instead of failing at run time. */
static const char *volatile cached_name;

static void remember(const char *name) {
    cached_name = name;
}

int main(void) {
    char *name = malloc(16);
    if (!name) return 1;
    strcpy(name, "worker");
    remember(name);

    free(name); /* the owner ends the lifetime */

    printf("cached name: %s\n", cached_name); /* BUG: heap-use-after-free */
    return 0;
}
