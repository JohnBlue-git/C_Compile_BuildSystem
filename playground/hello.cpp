// Replace or copy this file to try something out:
//   make run EX=pg_hello
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values{1, 2, 3};
    for (int value : values) {
        std::cout << value << ' ';
    }
    std::cout << '\n';
}
