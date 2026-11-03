#include <cstdio>
#include "helpers.h"
void call_from_a();
int main() {
    call_from_a();
    int s = next_static();
    int i = next_inline();
    std::printf("static: %d, inline: %d\n", s, i);
    return 0;
}
