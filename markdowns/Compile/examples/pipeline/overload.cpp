// overload.cpp - two functions with one name: only possible because of mangling.
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}
