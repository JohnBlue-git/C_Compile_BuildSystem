// Leftover '\n' after `>>`, and how ignore() fixes it.
// std::istringstream stands in for std::cin so the example runs unattended;
// both are std::istream, so the behavior is the same.
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

int main() {
    // The user typed "3<Enter>" and then "A<Enter>".
    {
        std::istringstream input("3\nA\n");
        int number = 0;
        char symbol = 0;
        input >> number;     // reads 3, leaves '\n' in the buffer
        input.get(symbol);   // get() does not skip whitespace: reads that '\n'
        std::cout << "without ignore(): symbol is "
                  << (symbol == '\n' ? "'\\n'" : std::string(1, symbol)) << '\n';
    }
    {
        std::istringstream input("3 trailing junk\nA\n");
        int number = 0;
        char symbol = 0;
        input >> number;
        // Discard the rest of the current line, including the '\n'.
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        input.get(symbol);
        std::cout << "with ignore():    symbol is '" << symbol << "'\n";
    }

    // >> word, then istream::getline(char*, n), then std::getline(string).
    std::istringstream input("hello world\nC style line\nstd::string line\n");
    std::string word, str;
    char cstr[100];
    input >> word;      // "hello" (stops at whitespace)
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // drop " world\n"
    input.getline(cstr, sizeof cstr);  // at most 99 chars; sets failbit if the line is longer
    std::getline(input, str);          // grows the string as needed
    std::cout << "word=\"" << word << "\" cstr=\"" << cstr << "\" str=\"" << str << "\"\n";
}
