// main.cpp - compiled by the C++ compiler; calls the C function add().
// Without the extern "C" block in add.h, this file would reference the
// mangled name _Z3addii and the link would fail with "undefined reference".
#include "add.h"

#include <iostream>

int main() {
    std::cout << "add(2, 3) = " << add(2, 3) << "\n";
    return add(2, 3) == 5 ? 0 : 1;
}
