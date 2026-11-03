// add_c_linkage.cpp - compiled as C++, but exported with the C symbol name.
extern "C" int add(int a, int b) {
    return a + b;
}
