/* leak.c - DELIBERATE BUG. Run it with the asan preset:
 *   make run PRESET=asan EX=memory_bug_leak
 *
 * make_greeting() returns heap memory, and the caller must free it. The
 * caller forgets, so LeakSanitizer reports the blocks at exit. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *make_greeting(const char *name) {
    char *text = malloc(strlen("Hello, ") + strlen(name) + 1);
    if (!text) exit(1);
    strcpy(text, "Hello, ");
    strcat(text, name);
    return text; /* the caller owns the result */
}

int main(void) {
    const char *names[] = {"alice", "bob", "carol"};
    for (int i = 0; i < 3; ++i) {
        char *greeting = make_greeting(names[i]);
        puts(greeting);
        /* BUG: missing free(greeting); */
    }
    return 0;
}
