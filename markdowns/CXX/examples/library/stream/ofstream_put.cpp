// Writing with << and put(), then reading the file back.
#include <fstream>
#include <iostream>
#include <string>

int main() {
    {
        std::ofstream out("output.txt");  // created in the working directory
        if (!out) {
            std::cerr << "cannot create output.txt\n";
            return 1;
        }
        out << "Hello, world!\n";  // formatted output
        out.put('A');              // one unformatted character
        out.put('\n');
        out.put('B');
    }   // destructor flushes and closes the file

    std::ifstream in("output.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::cout << line << '\n';
    }
}
