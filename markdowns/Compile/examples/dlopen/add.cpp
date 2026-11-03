// add.cpp - plugin without extern "C": the exported symbol is _Z3addii.
int add(int a, int b) {
    return a + b;
}
