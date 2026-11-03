// Stream state flags, the `while (!in.eof())` anti-pattern, and rewinding.
// data.txt contains: "1 2\n3 4\n5 6\n"
#include <fstream>
#include <iostream>

void printState(const std::ifstream& in) {
    std::cout << "  [good=" << in.good() << " fail=" << in.fail()
              << " bad=" << in.bad() << " eof=" << in.eof() << "]\n";
}

int main() {
    std::ifstream in("data.txt");
    if (!in) {  // same as in.fail()
        std::cerr << "cannot open data.txt\n";
        return 1;
    }

    // ANTI-PATTERN: eof() becomes true only AFTER a read hits the end.
    // After reading '6', eof() is still false, so the loop runs once more;
    // that read fails, `value` keeps its old content, and '6' prints twice.
    std::cout << "while (!in.eof()) -- wrong:\n";
    char value = 0;
    while (!in.eof()) {
        in >> value;
        std::cout << value;
        printState(in);
    }

    // Since C++11 seekg() clears eofbit, but NOT failbit, so clear() first.
    in.clear();
    in.seekg(0, std::ios::beg);

    // CORRECT: test the result of the read itself.
    std::cout << "\nwhile (in >> number) -- right:\n";
    int number = 0;
    while (in >> number) {
        std::cout << "  read " << number << '\n';
    }
    printState(in);  // fail=1 eof=1: the loop ended at end of file

    in.clear();
    in.seekg(0, std::ios::beg);
    std::cout << "\nafter clear() + seekg(0), first number again: ";
    in >> number;
    std::cout << number << '\n';
}   // ifstream closes the file in its destructor (RAII)
