/* stdio file I/O: fopen modes, fgets, fputs, fprintf, fseek/ftell, rewind.
 * Reads sample.txt and writes a separate output.txt (sample.txt is not changed).
 * Note: 06_file_io.md */
#include <stdio.h>
#include <stdlib.h>

/* Print every line; fgets keeps the '\n' and stops at sizeof buffer - 1. */
static int print_file(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return -1;
    }
    char buffer[256];
    while (fgets(buffer, sizeof buffer, file) != NULL) {
        fputs(buffer, stdout);
    }
    int failed = ferror(file);
    fclose(file);
    return failed ? -1 : 0;
}

static long file_size(const char *path) {
    /* "rb": for binary streams the byte offset from ftell is exact. */
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        return -1;
    }
    long size = -1;
    if (fseek(file, 0, SEEK_END) == 0) {
        size = ftell(file); /* current offset == size; -1 on error */
    }
    fclose(file);
    return size;
}

int main(void) {
    /* Show only the first 2 lines of sample.txt, then rewind and count lines. */
    FILE *sample = fopen("sample.txt", "r");
    if (sample == NULL) {
        perror("sample.txt");
        return EXIT_FAILURE;
    }
    char line[256];
    for (int i = 0; i < 2 && fgets(line, sizeof line, sample) != NULL; i++) {
        printf("sample.txt: %s", line);
    }
    rewind(sample); /* back to offset 0; only valid while the file is open */
    int lines = 0;
    while (fgets(line, sizeof line, sample) != NULL) {
        lines++; /* a line longer than the buffer counts more than once */
    }
    fclose(sample);
    printf("sample.txt: %d fgets() calls, %ld bytes\n\n", lines, file_size("sample.txt"));

    /* "w": create or truncate. */
    FILE *out = fopen("output.txt", "w");
    if (out == NULL) {
        perror("output.txt");
        return EXIT_FAILURE;
    }
    if (fputs("written with fputs\n", out) == EOF ||
        fprintf(out, "written with fprintf: %s is %d\n", "answer", 42) < 0) {
        perror("write output.txt");
        fclose(out);
        return EXIT_FAILURE;
    }
    fclose(out);

    /* "a": every write goes to the end of the file. */
    out = fopen("output.txt", "a");
    if (out == NULL) {
        perror("output.txt");
        return EXIT_FAILURE;
    }
    fputs("appended with mode \"a\"\n", out);
    if (fclose(out) == EOF) { /* fclose flushes; a write error can show up here */
        perror("fclose");
        return EXIT_FAILURE;
    }

    printf("output.txt (%ld bytes):\n", file_size("output.txt"));
    return print_file("output.txt") == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
