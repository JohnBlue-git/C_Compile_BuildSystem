// static_cast, dynamic_cast, const_cast, reinterpret_cast, C-style cast.
// The undefined-behavior cases are shown in comments only.
#include <bit>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <typeinfo>

// --- static_cast: reverse an implicit conversion, no runtime check ---
struct Job {
  int id;
};

// A C-style API that only knows void*.
void run_callback(void (*fn)(void*), void* user_data) { fn(user_data); }

void on_job(void* data) {
  // We know data came from a Job*, so no check is needed.
  auto* job = static_cast<Job*>(data);
  std::cout << "  static_cast<Job*>(void*) -> job " << job->id << '\n';
}

// --- dynamic_cast: checked downcast in a polymorphic hierarchy ---
struct Stmt {
  virtual ~Stmt() = default;  // at least one virtual function: polymorphic
};
struct JumpStmt : Stmt {
  const char* target = "loop_end";
};
struct ExprStmt : Stmt {
  int value = 7;
};

void inspect(Stmt& stmt) {
  if (auto* j = dynamic_cast<JumpStmt*>(&stmt)) {
    std::cout << "  JumpStmt to " << j->target << '\n';
  } else if (auto* e = dynamic_cast<ExprStmt*>(&stmt)) {
    std::cout << "  ExprStmt with value " << e->value << '\n';
  } else {
    std::cout << "  unknown Stmt\n";
  }
}

// Not polymorphic: dynamic_cast downcast is a compile error.
struct PlainBase {};
struct PlainDerived : PlainBase {};

// --- const_cast: legacy API that takes char* but does not write ---
std::size_t legacy_length(char* text) {
  std::size_t n = 0;
  while (text[n] != '\0') ++n;
  return n;
}

int main() {
  std::cout << "static_cast\n";
  double pi = 3.14159;
  int truncated = static_cast<int>(pi);
  std::cout << "  static_cast<int>(3.14159) = " << truncated << '\n';
  Job job{17};
  run_callback(on_job, &job);  // Job* -> void* is implicit

  std::cout << "dynamic_cast\n";
  JumpStmt jump;
  ExprStmt expr;
  Stmt plain;
  inspect(jump);
  inspect(expr);
  inspect(plain);
  try {
    Stmt& s = expr;
    [[maybe_unused]] auto& j = dynamic_cast<JumpStmt&>(s);  // reference: throws instead of null
  } catch (const std::bad_cast& e) {
    std::cout << "  dynamic_cast to reference failed: " << e.what() << '\n';
  }
  // PlainDerived d; PlainBase* b = &d;
  // dynamic_cast<PlainDerived*>(b);  // error: PlainBase is not polymorphic

  std::cout << "const_cast\n";
  const char* message = "hello";
  std::cout << "  legacy_length = " << legacy_length(const_cast<char*>(message)) << '\n';

  int mutable_value = 10;
  const int& view = mutable_value;
  const_cast<int&>(view) = 42;  // OK: the object itself is not const
  std::cout << "  object originally non-const, now " << mutable_value << '\n';

  // const int truly_const = 10;
  // *const_cast<int*>(&truly_const) = 100;  // UNDEFINED BEHAVIOR: object is const

  std::cout << "reinterpret_cast\n";
  std::uint32_t word = 0x01020304;
  // Viewing any object as bytes through unsigned char* is allowed.
  auto* bytes = reinterpret_cast<const unsigned char*>(&word);
  std::cout << "  first byte = " << static_cast<int>(bytes[0])
            << (bytes[0] == 4 ? " (little-endian)\n" : " (big-endian)\n");
  auto address = reinterpret_cast<std::uintptr_t>(&job);  // pointer -> integer
  auto* back = reinterpret_cast<Job*>(address);            // and back: same pointer
  std::cout << "  pointer round-trip ok: " << std::boolalpha << (back == &job) << '\n';
  // float f = *reinterpret_cast<float*>(&word);  // UB: strict aliasing
  float f = std::bit_cast<float>(word);           // C++20: well-defined type punning
  std::printf("  bit_cast<float>(0x01020304) = %g\n", static_cast<double>(f));

  std::cout << "C-style cast\n";
  int c_style = (int)pi;  // tries const_cast, static_cast, reinterpret_cast in turn
  std::cout << "  (int)pi = " << c_style << '\n';
  // const Job cj{1};
  // Stmt* nonsense = (Stmt*)&cj;  // compiles: silent reinterpret_cast + const_cast
}
