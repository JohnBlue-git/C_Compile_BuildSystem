#include "config.h"
const int *max_a(void);
int main(void) { return max_a() == &MAX; } /* C: never reached, link fails */
