// Lambda captures: by value, by reference, init-capture, mutable,
// std::function captured by copy/move, this vs shared_from_this.
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

class Worker : public std::enable_shared_from_this<Worker> {
public:
  static void static_func() { std::cout << "  static member: no this needed\n"; }
  void member_func() const { std::cout << "  member_func, var = " << var_ << '\n'; }

  // [this] captures the pointer only. Fine when the lambda runs
  // before this function returns (synchronous use).
  void run_now() {
    auto func = [this] {
      static_func();
      member_func();
      ++var_;  // modifies *this through the captured pointer
    };
    func();
  }

  // For work that may outlive the call, capture a shared_ptr so the
  // object stays alive until the lambda is done with it.
  void run_on_thread() {
    auto task = [self = shared_from_this()] { self->member_func(); };
    std::thread t(task);
    t.join();
  }

private:
  int var_ = 3;
};

int main() {
  std::cout << "1. capture const reference and pointer-to-const\n";
  int var1 = 3;
  const int& const_ref = var1;
  int var2 = 6;
  const int* const_ptr = &var2;
  auto add = [&const_ref, const_ptr](int num) {
    // const_ref = 1;  // error: refers to const int
    // *const_ptr = 1; // error: points to const int
    return const_ref + *const_ptr + num;
  };
  std::cout << "  add(9) = " << add(9) << '\n';

  std::cout << "2. [=] copies at creation, [&] sees later changes\n";
  int x = 10;
  int y = 20;
  auto by_value = [=] { return x + y; };
  auto by_ref = [&] { return x + y; };
  x = 100;
  std::cout << "  by_value() = " << by_value() << ", by_ref() = " << by_ref() << '\n';

  std::cout << "3. mixed capture and mutable\n";
  int a = 10;
  int b = 20;
  auto mixed = [a, &b] { b += a; };  // a is a const copy, b is the original
  mixed();
  std::cout << "  b = " << b << '\n';
  auto counter = [n = 0]() mutable { return ++n; };  // mutable: the copy may change
  counter();
  std::cout << "  counter() second call = " << counter() << '\n';

  std::cout << "4. std::function captured by copy and by move (init-capture)\n";
  std::function<void()> callback = [] { std::cout << "  callback called\n"; };
  auto by_copy = [callback] { callback(); };
  by_copy();
  auto by_move = [callback = std::move(callback)] { callback(); };
  by_move();
  std::cout << "  original after move: " << (callback ? "still set" : "empty")
            << " (valid but unspecified; do not rely on it)\n";

  std::cout << "5. nested move needs mutable\n";
  std::function<void()> cb1 = [] { std::cout << "  cb1 called\n"; };
  // Inside a non-mutable lambda the captured cb1 is const, so std::move(cb1)
  // would be const&& and silently COPY. mutable makes the move real.
  auto outer = [cb1]() mutable {
    auto inner = [cb1 = std::move(cb1)] { cb1(); };
    inner();
  };
  outer();

  std::cout << "6. member lambdas: [this] vs [self = shared_from_this()]\n";
  auto worker = std::make_shared<Worker>();
  worker->run_now();
  worker->run_on_thread();

  std::cout << "7. lambdas with algorithms\n";
  std::vector<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
  std::cout << "  ";
  std::for_each(numbers.begin(), numbers.end(), [](int n) { std::cout << n << ' '; });
  std::cout << '\n';
  int sum = 0;
  std::for_each(numbers.begin(), numbers.end(), [&sum](int n) { sum += n; });
  auto even = std::count_if(numbers.begin(), numbers.end(), [](int n) { return n % 2 == 0; });
  std::cout << "  sum = " << sum << ", even count = " << even << '\n';

  auto print = [](const auto& value) { std::cout << "  generic: " << value << '\n'; };
  print(42);
  print("text");
}
