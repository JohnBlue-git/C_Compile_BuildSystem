// std::optional (C++17): a type-safe "maybe a value" instead of sentinel
// values such as -1, nullptr, or a separate bool flag.
#include <functional>  // std::reference_wrapper, std::ref
#include <iostream>
#include <optional>
#include <string>

std::optional<std::string> findName(bool found) {
    if (found) {
        return "John Doe";
    }
    return std::nullopt;
}

void commonUsage() {
    auto name = findName(true);
    if (name) {  // same as name.has_value()
        std::cout << "Name: " << *name << '\n';         // unchecked access
        std::cout << "Name: " << name.value() << '\n';  // throws bad_optional_access if empty
    }

    name.reset();
    std::cout << "After reset: " << name.value_or("<none>") << '\n';

    try {
        std::cout << findName(false).value() << '\n';
    } catch (const std::bad_optional_access& e) {
        std::cout << "value() on empty optional: " << e.what() << '\n';
    }
}

// By value: the function works on its own copy of the optional (and string).
void byValue(std::optional<std::string> opt) {
    if (opt) {
        opt.value() = "changed inside byValue";
    }
}

// By reference: the caller's optional is modified.
void byReference(std::optional<std::string>& opt) {
    if (opt) {
        opt.value() = "changed inside byReference";
    }
}

// std::optional<T&> is ill-formed before C++26, so wrap the reference.
void appendSuffix(std::optional<std::reference_wrapper<std::string>> target) {
    if (target) {
        target->get() += " (processed)";
    } else {
        std::cout << "No string to process\n";
    }
}

int main() {
    commonUsage();

    std::optional<std::string> text = "original";
    byValue(text);
    std::cout << "After byValue:     " << *text << '\n';  // original
    byReference(text);
    std::cout << "After byReference: " << *text << '\n';  // changed

    std::string myString = "Hello";
    appendSuffix(std::ref(myString));
    std::cout << "After appendSuffix: " << myString << '\n';
    appendSuffix(std::nullopt);
}
