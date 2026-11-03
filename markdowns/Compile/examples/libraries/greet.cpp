// greet.cpp - C++ code that really uses libstdc++ (std::string).
#include <string>

extern "C" unsigned long greeting_length(const char *name) {
    return std::string("Hello, ").append(name).size();
}
