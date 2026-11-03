// Character-wise get() vs line-wise std::getline() on a file.
// data.txt contains: "1 2\n3 4\n5 6\n"
#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::ifstream in("data.txt");
    if (!in.is_open()) {
        std::cerr << "Error: failed to open data.txt\n";
        return 1;
    }

    // get(ch) reads every character, whitespace and '\n' included.
    std::cout << "--- get() ---\n";
    char ch;
    int chars = 0, newlines = 0;
    while (in.get(ch)) {
        ++chars;
        if (ch == '\n') {
            ++newlines;
        }
    }
    std::cout << chars << " characters, " << newlines << " newlines\n";
    std::cout << "eof=" << in.eof() << " fail=" << in.fail() << '\n';

    in.clear();
    in.seekg(0, std::ios::beg);

    // getline() reads up to '\n' and discards the '\n'.
    std::cout << "\n--- std::getline() ---\n";
    std::string line;
    while (std::getline(in, line)) {
        std::cout << "Line: \"" << line << "\"  [eof=" << in.eof() << "]\n";
    }
}
