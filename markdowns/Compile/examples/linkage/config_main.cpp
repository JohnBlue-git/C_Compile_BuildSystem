#include <cstdio>
#include "config.h"
const int *max_a();
int main() {
    // Each C++ TU has its own private MAX, so the addresses differ.
    std::printf("same object: %s\n", max_a() == &MAX ? "yes" : "no");
    return 0;
}
