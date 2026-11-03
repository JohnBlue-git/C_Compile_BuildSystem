/* main.c - calls add() from another translation unit. */
#include <stdio.h>

#define TWICE(x) ((x) * 2)

int add(int a, int b); /* declaration only: the linker must find the definition */

int main(void) {
    printf("%d\n", TWICE(add(2, 3)));
    return 0;
}
