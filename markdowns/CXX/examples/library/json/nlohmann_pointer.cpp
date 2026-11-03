// JSON Pointer (RFC 6901) in nlohmann/json.
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using namespace nlohmann::literals;  // for the _json_pointer literal

int main() {
    auto j = json::parse(R"({
        "array": ["A", "B", "C"],
        "nested": { "one": 1, "two": 2, "three": [true, false] }
    })");

    std::cout << "\"\" (root) = " << j[""_json_pointer].dump() << '\n';
    std::cout << "/nested/one = " << j["/nested/one"_json_pointer] << '\n';

    // A pointer built at run time; at() throws if the path does not exist.
    std::string path = "/nested/three/1";
    std::cout << path << " = " << j.at(json::json_pointer(path)) << '\n';

    // value() with a default for a missing path.
    std::cout << "/nested/four (default 0) = "
              << j.value(json::json_pointer("/nested/four"), 0) << '\n';

    // Writing through a pointer creates missing objects.
    j["/server/port"_json_pointer] = 8080;
    std::cout << "/server/port = " << j["/server/port"_json_pointer] << '\n';

    // flatten(): one level, keys are JSON Pointers. unflatten() reverses it.
    json flat = j.flatten();
    std::cout << "flattened = " << flat.dump() << '\n';
    std::cout << "round trip equal: " << std::boolalpha << (flat.unflatten() == j) << '\n';
}
