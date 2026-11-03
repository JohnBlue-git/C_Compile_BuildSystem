/* Error handling in C: return codes, errno + perror, the goto-cleanup
 * pattern, and assert for programmer errors.
 * Note: 05_error_handling_and_signals.md */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Acquire three resources; on any failure release what was acquired,
 * in reverse order, through a single exit path. Returns 0 or -1. */
static int process(const char *path) {
    int result = -1;
    char *buffer = NULL;
    FILE *file = NULL;

    buffer = malloc(256);
    if (buffer == NULL) {
        goto cleanup;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        perror(path); /* prints strerror(errno) */
        goto cleanup;
    }

    if (fgets(buffer, 256, file) == NULL) {
        goto cleanup;
    }
    printf("first line: %s", buffer);
    result = 0;

cleanup:
    if (file != NULL) {
        fclose(file);
    }
    free(buffer); /* free(NULL) is a no-op */
    return result;
}

static size_t count_chars(const char *text) {
    /* assert documents a precondition; it is removed when NDEBUG is defined,
     * so never put required work inside it. */
    assert(text != NULL);
    return strlen(text);
}

int main(void) {
    int ok = process("sample.txt");
    int missing = process("no_such_file.txt"); /* expected to fail cleanly */
    printf("process(sample.txt) = %d, process(no_such_file.txt) = %d\n", ok, missing);
    printf("count_chars(\"abc\") = %zu\n", count_chars("abc"));

    return (ok == 0 && missing == -1) ? EXIT_SUCCESS : EXIT_FAILURE;
}
