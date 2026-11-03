/* Function-like macros vs static inline functions, stringification (#),
 * token pasting (##), predefined macros, and conditional compilation.
 * Note: 04_qualifiers_and_preprocessor.md */
#include <stdio.h>

#define SQUARE(x) ((x) * (x))  /* parentheses around x and the whole result */
#define BAD_SQUARE(x) x * x    /* BAD_SQUARE(1 + 2) -> 1 + 2 * 1 + 2 */

#define TO_STRING(x) #x                  /* stringification */
#define MAKE_NAME(prefix, n) prefix##n   /* token pasting */

#define PRINT_INT(expr) printf("%s = %d\n", #expr, (expr))

/* static inline: a hint to inline; the compiler type-checks the argument
 * and evaluates it exactly once. Plain "inline int cube(int)" in C without
 * an external definition can fail to link at -O0. */
static inline int cube(int x) {
    return x * x * x;
}

int main(void) {
    int value_1 = 5;

    PRINT_INT(SQUARE(value_1));
    PRINT_INT(SQUARE(1 + 2));      /* 9 */
    PRINT_INT(BAD_SQUARE(1 + 2));  /* 5, not 9 */
    PRINT_INT(cube(value_1));
    PRINT_INT(MAKE_NAME(value_, 1)); /* expands to value_1 */
    printf("TO_STRING(a + b) -> \"%s\"\n", TO_STRING(a + b));

    printf("%s:%d in %s()\n", __FILE__, __LINE__, __func__);

#ifdef NDEBUG
    printf("NDEBUG is defined: assert() is disabled\n");
#else
    printf("NDEBUG is not defined: assert() is active\n");
#endif

#if __STDC_VERSION__ >= 201710L
    printf("compiled as C17 or later\n");
#endif
    return 0;
}
