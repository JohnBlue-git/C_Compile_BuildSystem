// Navigating nested Boost.JSON values: by hand, and with JSON Pointer
// (value::at_pointer / find_pointer, available since Boost 1.81).
#include <boost/json.hpp>
#include <cstdint>
#include <iostream>

namespace json = boost::json;

int main() {
    json::value jv = json::parse(R"({
        "array": ["A", "B", "C"],
        "nested": { "one": 1, "two": 2, "three": [true, false] }
    })");

    // By hand: one as_object()/as_array() per level.
    const json::object& obj = jv.as_object();
    std::int64_t one = obj.at("nested").as_object().at("one").as_int64();
    bool second = obj.at("nested").as_object().at("three").as_array().at(1).as_bool();
    std::cout << std::boolalpha;
    std::cout << "manual  /nested/one = " << one << ", /nested/three/1 = " << second << '\n';

    // JSON Pointer: at_pointer() throws if the path is missing.
    std::cout << "pointer /nested/one = " << jv.at_pointer("/nested/one")
              << ", /nested/three/1 = " << jv.at_pointer("/nested/three/1") << '\n';

    // find_pointer() reports a missing path through an error_code.
    boost::system::error_code ec;
    if (jv.find_pointer("/nested/four", ec) == nullptr) {
        std::cout << "/nested/four missing: " << ec.message() << '\n';
    }
}
