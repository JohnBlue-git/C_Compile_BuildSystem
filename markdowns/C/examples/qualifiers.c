/* const with pointers, static local variables, and static functions.
 * Modifying a const object through a cast is undefined behavior, so this
 * example only shows what the compiler rejects (see the comments).
 * Note: 04_qualifiers_and_preprocessor.md */
#include <stdio.h>

/* static at file scope: internal linkage, invisible to other .c files. */
static int next_id(void) {
    /* static local: initialized once, keeps its value between calls. */
    static int counter = 0;
    return ++counter;
}

int main(void) {
    int a = 1;
    int b = 2;

    const int *to_const = &a;       /* cannot modify *to_const */
    /* *to_const = 5;                  error: read-only location */
    to_const = &b;                  /* OK: the pointer itself can change */

    int *const const_ptr = &a;      /* cannot change where it points */
    *const_ptr = 10;                /* OK: the int can change */
    /* const_ptr = &b;                 error: read-only variable */

    const int *const both = &b;     /* neither can change */

    printf("*to_const = %d, a = %d, *both = %d\n", *to_const, a, *both);

    /* next_id() returns 1, 2, 3: counter survives between calls. */
    for (int i = 0; i < 3; i++) {
        printf("next_id() = %d\n", next_id());
    }

    /* volatile: every access is a real load/store; the compiler may not
     * cache the value in a register or remove "redundant" reads. */
    volatile int status = 0;
    status = 1;
    printf("volatile status = %d\n", status);
    return 0;
}
