// Containers cannot hold references (std::vector<int&> is ill-formed).
// std::reference_wrapper<T> is a copyable, rebindable reference that can.
#include <functional>  // std::reference_wrapper
#include <iostream>
#include <vector>

void print(const char* label, const std::vector<int>& values) {
    std::cout << label;
    for (int v : values) {
        std::cout << ' ' << v;
    }
    std::cout << '\n';
}

int main() {
    std::vector<int> values{1, 2, 3, 4, 5};
    print("values:", values);

    // Each element refers to an element of `values`.
    // (refs.insert(refs.end(), values.begin(), values.end()) works too.)
    std::vector<std::reference_wrapper<int>> refs(values.begin(), values.end());

    for (int& v : refs) {  // reference_wrapper<int> converts to int&
        v <<= 1;
    }
    print("values after doubling through refs:", values);

    // Danger: refs dangle if `values` reallocates or is destroyed.
}
