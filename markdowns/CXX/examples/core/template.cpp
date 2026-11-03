// Function templates: one definition, one instantiation per deduced type.
#include <cstddef>
#include <iostream>
#include <string>

template <typename T>
T max_value(T a, T b) {
  return (a > b) ? a : b;
}

// Non-type template parameter: N is deduced from the array type.
template <typename T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) {
  return N;
}

int main() {
  std::cout << "max_value(3, 7)       = " << max_value(3, 7) << '\n';
  std::cout << "max_value(4.56, 2.34) = " << max_value(4.56, 2.34) << '\n';
  std::cout << "max_value('a', 'b')   = " << max_value('a', 'b') << '\n';

  // max_value(3, 4.5);            // error: T deduced as both int and double
  std::cout << "max_value<double>(3, 4.5) = " << max_value<double>(3, 4.5) << '\n';

  // Without <std::string>, T would be const char* and compare addresses.
  std::cout << "max_value<std::string>(\"apple\", \"pear\") = "
            << max_value<std::string>("apple", "pear") << '\n';

  int values[4]{};
  static_assert(array_size(values) == 4);
  std::cout << "array_size(values) = " << array_size(values) << '\n';
}
