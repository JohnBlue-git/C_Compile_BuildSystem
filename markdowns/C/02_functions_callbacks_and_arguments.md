# C Notes 02: Functions, Callbacks, and Argument Passing

## Function pointers and callbacks

Function pointers are a good way to inject a strategy or register a callback, but the API must define the callback's lifetime and its user data. A typical C interface also passes a `void *context`, so state does not have to live in global variables.

```c
typedef void (*event_callback)(const char *event, void *context);

void emit_event(const char *event,
                event_callback callback,
                void *context) {
    if (callback != NULL) {
        callback(event, context);
    }
}

static void print_event(const char *event, void *context) {
    const char *prefix = context;
    printf("%s: %s\n", prefix, event);
}

/* emit_event("connected", print_event, "network"); */
```

A callback API should state whether the callback is stored, when it is called, who owns `context`, and whether the callback may modify or free `context` while it runs.

## Argument passing in C: by value and by pointer

C only has pass by value. Even when the parameter is a pointer, the function receives a copy of the pointer value. Through it the function can modify the caller's data, but assigning a new value to the pointer does not change the caller's pointer.

```c
void set_value(int *value) {
    *value = 42;                 /* modifies the int the caller owns */
    value = NULL;                /* modifies only the local copy of the pointer */
}

void swap(int *left, int *right) {
    int temporary = *left;
    *left = *right;
    *right = temporary;
}
```

If the function must change the caller's pointer itself, for example to allocate memory and hand back a new owner, pass a pointer to the pointer:

```c
int allocate_value(int **out_value) {
    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return -1;
    }
    *value = 42;
    *out_value = value;
    return 0;
}
```

Run it: `make run EX=c_callbacks` ([examples/callbacks.c](examples/callbacks.c))

Passing an `int **` row table to a function (as in [00](00_fundamentals.md#dynamic-2d-arrays)) follows the same rule: the function can change `rows[i][j]`, but not the caller's `rows` variable.
