// Nested try/catch, rethrow with `throw;`, and why `throw e;` slices.
#include <iostream>
#include <stdexcept>

struct ConfigError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

void inner_function() { throw ConfigError("missing key 'port'"); }

// Log, then rethrow the ORIGINAL exception object.
void outer_rethrow() {
  try {
    inner_function();
  } catch (const std::exception& e) {
    std::cout << "  outer: caught \"" << e.what() << "\", rethrowing with throw;\n";
    throw;
  }
}

// Log, then throw a COPY of the static type: ConfigError is sliced off.
void outer_throw_copy() {
  try {
    inner_function();
  } catch (const std::exception& e) {
    std::cout << "  outer: caught \"" << e.what() << "\", rethrowing with throw e;\n";
    throw e;  // copies a std::exception, not a ConfigError
  }
}

void report(void (*fn)()) {
  try {
    fn();
  } catch (const ConfigError& e) {
    std::cout << "  main: caught ConfigError: " << e.what() << '\n';
  } catch (const std::exception& e) {
    std::cout << "  main: caught only std::exception: " << e.what() << '\n';
  } catch (...) {
    std::cout << "  main: caught an unknown exception\n";
  }
}

int main() {
  std::cout << "throw;\n";
  report(outer_rethrow);
  std::cout << "throw e;\n";
  report(outer_throw_copy);
  std::cout << "throw 42;\n";
  report([] { throw 42; });  // any type can be thrown; catch (...) gets it
}
