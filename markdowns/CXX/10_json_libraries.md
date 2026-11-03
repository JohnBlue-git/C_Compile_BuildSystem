# C++ Interview Notes 10: JSON Libraries (nlohmann/json and Boost.JSON)

The standard library has no JSON support. The two common choices:

| | nlohmann/json | Boost.JSON |
|---|---|---|
| Distribution | header-only (one header, or the CMake package) | compiled library `Boost::json` (header-only mode also available) |
| Minimum standard | C++11 | C++11 (Boost 1.75+) |
| Value type | `nlohmann::json` (one type for every kind) | `boost::json::value` holding `object`, `array`, `string`, `int64`, `uint64`, `double`, `bool`, `null` |
| Feel | very convenient, STL-like, implicit conversions | explicit kinds (`as_object()`, `as_int64()`), faster parsing, allocator control |
| Pretty printing | `dump(4)` | `serialize()` is compact only |
| JSON Pointer (RFC 6901) | `json_pointer`, `"/a/b"_json_pointer` | `value::at_pointer`, `find_pointer` (Boost 1.81+) |
| Errors | exceptions (`json::parse_error`, `json::type_error`, `json::out_of_range`) | exceptions, or `error_code` overloads |

In this repo's Docker image both come from apt (nlohmann/json 3.11.3, Boost 1.83). CMake usage:

```cmake
find_package(nlohmann_json 3 CONFIG REQUIRED)
target_link_libraries(app PRIVATE nlohmann_json::nlohmann_json)

find_package(Boost 1.83 CONFIG REQUIRED COMPONENTS json)
target_link_libraries(app PRIVATE Boost::json)
```

Prefer the package over copying `json.hpp` into the source tree: the version is visible and upgrades are one line. (The repo's examples use `QUIET` and skip the JSON targets when a package is missing.)

## 1. nlohmann/json basics

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json person;
person["name"] = "Alice";                      // creates an object on first use
person["age"] = 30;
person["skills"] = {"C++", "Python", "Rust"};  // initializer list -> array

std::string text = person.dump(4);             // serialize, 4-space indent
std::ofstream("person.json") << text;

std::ifstream in("person.json");
json loaded = json::parse(in);                 // or: in >> loaded;

std::string name = loaded["name"].get<std::string>();
int age = loaded.at("age").get<int>();
std::string email = loaded.value("email", "<none>");   // default if missing
```

Interview-relevant details:

- **`operator<<` prints JSON, not the raw value**: `std::cout << loaded["name"]` prints `"Alice"` with quotes. Use `get<std::string>()` for the C++ value.
- **`operator[]` vs `at()`**: on a non-const object, `j["missing"]` *inserts* a `null` (like `std::map::operator[]`); on a const object it is undefined behavior for a missing key. `at()` throws `json::out_of_range`. `value(key, default)` returns a default.
- **Type mismatch throws**: `j["name"].get<int>()` throws `json::type_error`.
- **Object key order**: `nlohmann::json` stores objects in a `std::map`, so keys come out sorted, not in insertion order. Use `nlohmann::ordered_json` to keep insertion order.
- **Your own types**: define `to_json`/`from_json`, or use `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Person, name, age)`, then `json j = person; auto p = j.get<Person>();`.

Run it: `make run EX=cxx_json_nlohmann` (writes `person.json` into the test's working directory).

## 2. JSON Pointer in nlohmann/json

A JSON Pointer ([RFC 6901](https://datatracker.ietf.org/doc/html/rfc6901)) is a string path such as `/nested/three/1`: object keys and array indices separated by `/`. `""` is the whole document. (`~1` escapes `/` and `~0` escapes `~` inside a key.)

Why use it:

1. **Readable path access**: `j["/nested/config/enabled"_json_pointer]` instead of `j["nested"]["config"]["enabled"]`.
2. **Paths built at run time**: `json::json_pointer ptr(path); j[ptr] = true;`, for example from a config file or a CLI argument.
3. **Standard format**: the same paths work in other languages and in JSON Patch (RFC 6902).
4. **Defaults for missing paths**: `j.value("/server/port"_json_pointer, 8080)`.
5. **Flatten / unflatten**: `j.flatten()` turns a nested document into one object whose keys are JSON Pointers (`{"/a/b": 1}`); `unflatten()` reverses it.

```cpp
using namespace nlohmann::literals;            // for _json_pointer

auto one   = j["/nested/one"_json_pointer];                 // 1
auto flag  = j.at(json::json_pointer("/nested/three/1"));    // throws if missing
auto four  = j.value(json::json_pointer("/nested/four"), 0); // 0 (default)
j["/server/port"_json_pointer] = 8080;         // creates the missing "server" object
json flat  = j.flatten();
```

Run it: `make run EX=cxx_json_nlohmann_pointer`

## 3. Boost.JSON basics

```cpp
#include <boost/json.hpp>
namespace json = boost::json;

json::object obj;
obj["name"] = "Alice";
obj["age"] = 30;
obj["skills"] = {"C++", "Python", "Boost"};

std::string text = json::serialize(obj);         // compact; no pretty printer

json::value parsed = json::parse(text);          // throws boost::system::system_error
const json::object& o = parsed.as_object();      // as_*() throws if the kind is wrong
std::int64_t age = o.at("age").as_int64();
std::string name = json::value_to<std::string>(o.at("name"));
if (auto* email = o.if_contains("email")) { /* ... */ }      // nullptr if missing
```

- Each `value` has a **kind**; access is explicit: `as_object()`, `as_array()`, `as_string()`, `as_int64()`, `as_bool()`. `as_*` throws on the wrong kind; `if_*` returns a pointer or `nullptr`; `get_*` is unchecked.
- `json::object::operator[]` inserts a `null` for a missing key, the same trap as `std::map`.
- `operator<<` on a `json::value` or `json::string` prints JSON (with quotes). Use `value_to<std::string>()` or the string's conversion to `std::string_view` for the raw text.
- `value_from` / `value_to` with `tag_invoke` overloads convert your own types.
- For error codes instead of exceptions: `json::parse(text, ec)`.

Run it: `make run EX=cxx_json_boost` (writes `person_boost.json` into the test's working directory).

## 4. Navigating nested Boost.JSON values

By hand, every level needs its own `as_object()` / `as_array()`:

```cpp
std::int64_t one = jv.as_object().at("nested").as_object().at("one").as_int64();
```

Boost.JSON **does** support JSON Pointer since Boost 1.81 (older notes that say otherwise predate it):

```cpp
jv.at_pointer("/nested/three/1");              // throws if the path is missing

boost::system::error_code ec;
if (jv.find_pointer("/nested/four", ec) == nullptr) {
    // ec says why
}
```

Boost 1.83 (the version in the Docker image) also has `value::set_at_pointer` to write through a pointer. nlohmann's `flatten`/`unflatten` and the `_json_pointer` literal have no Boost.JSON equivalent.

Run it: `make run EX=cxx_json_boost_pointer`

## 5. Choosing one

- **nlohmann/json**: convenience matters more than raw speed; config files, tests, tools, small services. Very easy to read and write.
- **Boost.JSON**: parsing speed and memory control matter (custom allocators, `monotonic_resource`, incremental `stream_parser`), or the project already uses Boost.
- For very high throughput, specialized parsers such as simdjson or RapidJSON are faster than both.
