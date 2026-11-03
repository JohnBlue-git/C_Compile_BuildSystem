// add_c_linkage.cpp - plugin with extern "C": the exported symbol is add.
extern "C" int add(int a, int b) {
    return a + b;
}
