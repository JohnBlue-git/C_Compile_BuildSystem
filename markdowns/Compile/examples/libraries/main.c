/* main.c - uses add() from libadd. */
#include <stdio.h>

int add(int a, int b);

int main(void) {
    printf("add(2, 3) = %d\n", add(2, 3));
    return 0;
}
