// RAII: a destructor runs when its object leaves scope,
// including when the scope is left by an exception (stack unwinding).
#include <cstdio>
#include <stdexcept>

class Tracer {
public:
  explicit Tracer(const char* name) : name_(name) { std::printf("acquire %s\n", name_); }
  ~Tracer() { std::printf("release %s\n", name_); }

  Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;

private:
  const char* name_;
};

void may_throw() {
  Tracer guard("guard in may_throw");
  throw std::runtime_error("boom");
  // No cleanup code needed here: ~Tracer() runs during unwinding.
}

int main() {
  std::puts("before scope");
  {
    Tracer t("t");
  }  // ~Tracer() runs here
  std::puts("after scope");
  // t = {};  // compile error: t no longer exists

  try {
    may_throw();
  } catch (const std::exception& e) {
    std::printf("caught: %s\n", e.what());
  }
}
