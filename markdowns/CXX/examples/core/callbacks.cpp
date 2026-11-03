// Callbacks: std::function (type erasure) vs a template parameter,
// and `using` vs `typedef` for the callback type.
#include <functional>
#include <iostream>
#include <vector>

using Callback = std::function<int(int)>;     // same as the typedef below
typedef std::function<int(int)> CallbackOld;  // C-style spelling

// Only `using` can declare an alias template.
template <typename T>
using Vec = std::vector<T>;

// Accepts any callable with a matching signature; one non-template function.
void process_numbers(const Callback& callback) {
  std::cout << "  std::function: " << callback(9) << '\n';
}

// Template: the exact callable type is known, so the call can be inlined.
template <typename F>
void process_numbers_fast(F&& callback) {
  std::cout << "  template:      " << callback(9) << '\n';
}

int triple(int n) { return n * 3; }

struct Multiplier {
  int factor;
  int scale(int n) const { return n * factor; }
};

int main() {
  int var1 = 3;
  int var2 = 6;
  auto lambda = [&var1, &var2](int num) { return var1 + var2 + num; };

  std::cout << "lambda:\n";
  process_numbers(lambda);
  process_numbers_fast(lambda);

  std::cout << "free function:\n";
  process_numbers(triple);
  process_numbers_fast(triple);

  std::cout << "member function (bind the object with a lambda):\n";
  Multiplier m{5};
  process_numbers([&m](int n) { return m.scale(n); });

  std::cout << "store several callbacks:\n";
  Vec<CallbackOld> pipeline = {triple, lambda, [](int n) { return -n; }};
  for (const auto& cb : pipeline) process_numbers(cb);

  Callback empty;
  try {
    empty(1);
  } catch (const std::bad_function_call& e) {
    std::cout << "calling an empty std::function throws: " << e.what() << '\n';
  }
}
