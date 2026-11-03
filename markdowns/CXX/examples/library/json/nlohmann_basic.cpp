// nlohmann/json: build, serialize, write to a file, read back.
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

int main() {
    json person;
    person["name"] = "Alice";
    person["age"] = 30;
    person["skills"] = {"C++", "Python", "Rust"};

    std::cout << "Serialized:\n" << person.dump(4) << '\n';  // pretty print, 4 spaces

    {
        std::ofstream out("person.json");  // written to the working directory
        out << person.dump(4);
    }

    std::ifstream in("person.json");
    json loaded = json::parse(in);  // throws json::parse_error on bad input

    // operator<< on a json value prints JSON, so strings keep their quotes.
    std::cout << "\nName (json):   " << loaded["name"] << '\n';
    // get<T>() converts to a C++ type.
    std::cout << "Name (string): " << loaded["name"].get<std::string>() << '\n';
    std::cout << "Age:           " << loaded.at("age").get<int>() << '\n';
    std::cout << "First skill:   " << loaded["skills"][0].get<std::string>() << '\n';

    // at() throws on a missing key; value() returns a default.
    std::cout << "Email:         " << loaded.value("email", "<none>") << '\n';
    try {
        loaded.at("email");
    } catch (const json::out_of_range& e) {
        std::cout << "at(\"email\") threw: " << e.what() << '\n';
    }
}
