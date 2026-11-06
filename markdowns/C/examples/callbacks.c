/* C notes 02: function-pointer callbacks with a context, pass by value/pointer,
 * and pointer-to-pointer output parameters. */
#include <stdio.h>
#include <stdlib.h>

typedef void (*event_callback)(const char *event, void *context);

static void emit_event(const char *event, event_callback callback, void *context) {
    if (callback != NULL) {
        callback(event, context);
    }
}

static void print_event(const char *event, void *context) {
    const char *prefix = context;
    printf("%s: %s\n", prefix, event);
}

/* The context carries state, so no global variable is needed. */
static void count_event(const char *event, void *context) {
    int *count = context;
    ++*count;
    printf("counted '%s' (%d so far)\n", event, *count);
}

static void set_value(int *value) {
    *value = 42;  /* modifies the int the caller owns */
    value = NULL; /* modifies only the local copy of the pointer */
    (void)value;
}

static void swap(int *left, int *right) {
    int temporary = *left;
    *left = *right;
    *right = temporary;
}

static int allocate_value(int **out_value) {
    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return -1;
    }
    *value = 42;
    *out_value = value;
    return 0;
}

int main(void) {
    printf("--- callbacks with a context ---\n");
    emit_event("connected", print_event, "network");
    int count = 0;
    emit_event("click", count_event, &count);
    emit_event("click", count_event, &count);
    emit_event("ignored", NULL, NULL); /* no callback registered: nothing happens */

    printf("--- pass by value and by pointer ---\n");
    int x = 0;
    int *p = &x;
    set_value(p);
    printf("x = %d, caller's pointer still points to x: %s\n", x, p == &x ? "yes" : "no");

    int a = 1, b = 2;
    swap(&a, &b);
    printf("after swap: a = %d, b = %d\n", a, b);

    printf("--- pointer to pointer ---\n");
    int *owned = NULL;
    if (allocate_value(&owned) != 0) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    printf("allocate_value set the caller's pointer: *owned = %d\n", *owned);
    free(owned);
    return EXIT_SUCCESS;
}
