// Boost.JSON: build, serialize, write to a file, read back.
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace json = boost::json;

int main() {
    json::object obj;
    obj["name"] = "Alice";
    obj["age"] = 30;
    obj["skills"] = {"C++", "Python", "Boost"};  // becomes a json::array

    {
        std::ofstream out("person_boost.json");  // written to the working directory
        out << json::serialize(obj);            // compact, no pretty printer
    }

    std::ifstream in("person_boost.json");
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::cout << "File content: " << text << '\n';

    json::value parsed = json::parse(text);  // throws boost::system::system_error on bad input
    const json::object& o = parsed.as_object();  // as_*() throws if the kind is wrong

    // operator<< prints JSON, so a json::string keeps its quotes;
    // value_to<T> (or json::string's conversion to string_view) gives the raw text.
    std::cout << "Name (json): " << o.at("name") << '\n';
    std::cout << "Name:        " << std::string_view(o.at("name").as_string()) << '\n';
    std::cout << "Age:         " << o.at("age").as_int64() << '\n';
    std::cout << "First skill: " << json::value_to<std::string>(o.at("skills").at(0)) << '\n';

    // if_contains() returns nullptr instead of throwing.
    if (o.if_contains("email") == nullptr) {
        std::cout << "Email:       <none>\n";
    }
}
