/* memset / memcpy / memmove / strncpy / snprintf.
 * Note: 03_strings_memory_and_copy.md */
#include <stdio.h>
#include <string.h>

int main(void) {
    const char source[] = "Hello, world!";
    /* sizeof on an array includes the '\0'; strlen does not. */
    printf("sizeof source = %zu, strlen(source) = %zu\n", sizeof source, strlen(source));

    /* memset: fill bytes, then terminate by hand. */
    char stars[sizeof source];
    memset(stars, '*', sizeof stars - 1);
    stars[sizeof stars - 1] = '\0';
    printf("memset : '%s'\n", stars);

    /* memcpy: copies exactly N bytes and knows nothing about '\0'.
     * Copy sizeof source so the terminator comes along. */
    char copy[sizeof source];
    memcpy(copy, source, sizeof source);
    printf("memcpy : '%s'\n", copy);

    /* Works for any object type, not only strings. */
    int numbers[3] = {1, 2, 3};
    int numbers_copy[3];
    memcpy(numbers_copy, numbers, sizeof numbers);
    printf("memcpy int[3]: %d %d %d\n", numbers_copy[0], numbers_copy[1], numbers_copy[2]);

    /* memmove: source and destination may overlap (memcpy would be UB here).
     * Shift "abcdef" right by 2 inside the same buffer. */
    char overlap[16] = "abcdef";
    memmove(overlap + 2, overlap, strlen(overlap) + 1);
    printf("memmove: '%s'\n", overlap); /* ababcdef */

    /* strncpy into a buffer that is too small: no '\0' is written,
     * so terminate by hand. */
    char small[6];
    strncpy(small, source, sizeof small);
    small[sizeof small - 1] = '\0';
    printf("strncpy: '%s' (truncated)\n", small);

    /* snprintf: always terminates (capacity > 0) and reports truncation. */
    char out[6];
    int needed = snprintf(out, sizeof out, "%s", source);
    if (needed < 0 || (size_t)needed >= sizeof out) {
        printf("snprintf: '%s' (needed %d chars, truncated)\n", out, needed);
    }
    return 0;
}
