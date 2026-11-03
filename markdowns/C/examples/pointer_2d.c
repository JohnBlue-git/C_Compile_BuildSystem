/* 2D array as an array of row pointers: malloc vs calloc, passing int **,
 * and freeing in the reverse order of allocation.
 * Note: 00_fundamentals.md */
#include <stdio.h>
#include <stdlib.h>

/* Free rows [0, rows) and the row table. free(NULL) is a no-op,
 * so a partially built table can be released safely. */
static void free_2d(int **array, int rows) {
    if (array == NULL) {
        return;
    }
    for (int i = 0; i < rows; i++) {
        free(array[i]);
    }
    free(array);
}

/* zeroed == 0: malloc (contents indeterminate); zeroed != 0: calloc (all zero). */
static int **alloc_2d(int rows, int cols, int zeroed) {
    /* calloc for the row table so every row pointer starts as NULL. */
    int **array = calloc((size_t)rows, sizeof *array);
    if (array == NULL) {
        return NULL;
    }
    for (int i = 0; i < rows; i++) {
        array[i] = zeroed ? calloc((size_t)cols, sizeof *array[i])
                          : malloc((size_t)cols * sizeof *array[i]);
        if (array[i] == NULL) {
            free_2d(array, i); /* do not leak the rows already allocated */
            return NULL;
        }
    }
    return array;
}

/* The function gets a copy of the pointer, but through it can modify
 * the caller's elements. */
static void fill(int **array, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            array[i][j] = i * cols + j;
        }
    }
}

static void double_all(int **array, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            array[i][j] *= 2;
        }
    }
}

static void print_2d(const char *label, int **array, int rows, int cols) {
    printf("%s:\n", label);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%3d", array[i][j]);
        }
        printf("\n");
    }
}

int main(void) {
    const int rows = 3;
    const int cols = 3;

    int **with_malloc = alloc_2d(rows, cols, 0);
    int **with_calloc = alloc_2d(rows, cols, 1);
    if (with_malloc == NULL || with_calloc == NULL) {
        fprintf(stderr, "allocation failed\n");
        free_2d(with_malloc, rows);
        free_2d(with_calloc, rows);
        return EXIT_FAILURE;
    }

    /* Reading with_malloc before writing it would read indeterminate values. */
    fill(with_malloc, rows, cols);
    print_2d("malloc + fill", with_malloc, rows, cols);
    print_2d("calloc (zeroed)", with_calloc, rows, cols);

    double_all(with_malloc, rows, cols);
    print_2d("malloc after double_all", with_malloc, rows, cols);

    free_2d(with_malloc, rows);
    free_2d(with_calloc, rows);
    return EXIT_SUCCESS;
}
