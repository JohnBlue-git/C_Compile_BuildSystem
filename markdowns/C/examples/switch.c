/* switch: case labels, break, grouped cases (fall-through), and default.
 * Runs over fixed values instead of reading stdin.
 * Note: 00_fundamentals.md */
#include <stdio.h>

static void describe(int choice) {
    switch (choice) {
    case 1:
        printf("%d: one\n", choice);
        break;
    case 2:
        printf("%d: two\n", choice);
        break;
    case 3:
    case 4: /* empty case falls through: 3 and 4 share one body */
        printf("%d: three or four\n", choice);
        break;
    default:
        printf("%d: invalid choice\n", choice);
        break;
    }
}

int main(void) {
    const int choices[] = {1, 2, 3, 4, 7};
    for (size_t i = 0; i < sizeof choices / sizeof choices[0]; i++) {
        describe(choices[i]);
    }
    return 0;
}
