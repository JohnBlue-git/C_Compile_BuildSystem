// std::move, move constructor / assignment, moved-from state,
// noexcept and vector reallocation, std::forward.
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Owns a heap string through a raw pointer, so it must define its own
// move operations (Rule of Five). Copy is disabled to keep the demo short.
class Greeter {
public:
  Greeter() : world_(new std::string(" World")) { std::cout << "  Greeter()\n"; }
  ~Greeter() { delete world_; }

  Greeter(const Greeter&) = delete;
  Greeter& operator=(const Greeter&) = delete;

  // Steal the pointer, then leave the source empty but valid,
  // so its destructor does not delete the same string again.
  Greeter(Greeter&& other) noexcept : world_(other.world_) {
    other.world_ = nullptr;
    std::cout << "  Greeter(Greeter&&)\n";
  }

  Greeter& operator=(Greeter&& other) noexcept {
    std::cout << "  operator=(Greeter&&)\n";
    if (this != &other) {
      delete world_;  // release what we own first
      world_ = other.world_;
      other.world_ = nullptr;
    }
    return *this;
  }

  void hello() const { std::cout << "  Hello" << (world_ ? *world_ : std::string{}) << '\n'; }

private:
  std::string* world_;
};

// Counts copies vs moves when std::vector grows.
struct Counted {
  static inline int copies = 0;
  static inline int moves = 0;
  Counted() = default;
  Counted(const Counted&) { ++copies; }
  Counted(Counted&&) noexcept { ++moves; }
};
struct CountedMayThrow {
  static inline int copies = 0;
  static inline int moves = 0;
  CountedMayThrow() = default;
  CountedMayThrow(const CountedMayThrow&) { ++copies; }
  CountedMayThrow(CountedMayThrow&&) { ++moves; }  // not noexcept
};

template <typename T>
void grow(int n) {
  std::vector<T> v;
  for (int i = 0; i < n; ++i) v.emplace_back();
}

void sink(const std::string&) { std::cout << "  sink(const std::string&) - lvalue\n"; }
void sink(std::string&&) { std::cout << "  sink(std::string&&)      - rvalue\n"; }

// Forwarding reference: T&& in a deduced context. std::forward keeps
// the caller's value category; std::move would always make it an rvalue.
template <typename T>
void relay(T&& arg) {
  sink(std::forward<T>(arg));
}

int main() {
  std::cout << "1. move constructor and moved-from object\n";
  Greeter a;
  Greeter b(std::move(a));  // std::move only casts; the move ctor does the work
  a.hello();                // valid but empty: prints "Hello"
  b.hello();                // prints "Hello World"

  std::cout << "2. move assignment\n";
  Greeter c;
  c = std::move(b);
  c.hello();

  std::cout << "3. std::vector move: O(1), source left valid but unspecified\n";
  std::vector<int> source = {1, 2, 3, 4, 5};
  std::vector<int> destination = std::move(source);
  std::cout << "  destination.size() = " << destination.size() << '\n';
  source.clear();  // reuse after move: assign or clear first, then use
  source.push_back(9);
  std::cout << "  source reused, size = " << source.size() << '\n';

  std::cout << "4. std::move on a const object silently copies\n";
  const std::string frozen = "still here";
  std::string taken = std::move(frozen);  // const std::string&& binds to the copy ctor
  std::cout << "  frozen = \"" << frozen << "\", taken = \"" << taken << "\"\n";

  std::cout << "5. vector growth: noexcept move vs throwing move (1000 push)\n";
  grow<Counted>(1000);
  grow<CountedMayThrow>(1000);
  std::cout << "  noexcept move : copies = " << Counted::copies << ", moves = " << Counted::moves << '\n';
  std::cout << "  may-throw move: copies = " << CountedMayThrow::copies
            << ", moves = " << CountedMayThrow::moves << '\n';

  std::cout << "6. perfect forwarding\n";
  std::string name = "lvalue";
  relay(name);                   // T = std::string&  -> lvalue overload
  relay(std::string("rvalue"));  // T = std::string   -> rvalue overload
}
