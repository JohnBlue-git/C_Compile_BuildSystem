// Virtual dispatch vs CRTP in a tight loop.
// Only meaningful in an optimized build: make run PRESET=release EX=cxx_crtp_benchmark
#include <chrono>
#include <iostream>
#include <memory>

// --- Dynamic polymorphism ---
struct BaseVirtual {
  virtual ~BaseVirtual() = default;
  virtual void tick() = 0;
  long long count = 0;
};
struct IncVirtual final : BaseVirtual {
  void tick() override { ++count; }
};
struct DecVirtual final : BaseVirtual {
  void tick() override { --count; }
};

// --- Static polymorphism (CRTP) ---
template <typename Derived>
struct BaseCRTP {
  void tick() { static_cast<Derived*>(this)->tick_impl(); }
  long long count = 0;
};
struct IncCRTP : BaseCRTP<IncCRTP> {
  void tick_impl() { ++count; }
};

template <typename F>
double seconds_for(F&& body) {
  const auto start = std::chrono::steady_clock::now();
  body();
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
  return elapsed.count();
}

int main(int argc, char**) {
  constexpr int iterations = 50'000'000;

  // The dynamic type depends on argc, so the optimizer cannot prove which
  // tick() runs and must keep the indirect call (no devirtualization).
  std::unique_ptr<BaseVirtual> v;
  if (argc > 1) {
    v = std::make_unique<DecVirtual>();
  } else {
    v = std::make_unique<IncVirtual>();
  }
  const double virtual_s = seconds_for([&] {
    for (int i = 0; i < iterations; ++i) v->tick();
  });

  IncCRTP c;
  const double crtp_s = seconds_for([&] {
    for (int i = 0; i < iterations; ++i) c.tick();
  });

  // Printing the counters keeps both results observable. In a release build
  // the CRTP loop is inlined and folded into "count += iterations", so its
  // time is near zero: that is the inlining benefit, not a per-call cost.
  constexpr double ns = 1e9 / iterations;
  std::cout << "virtual: " << virtual_s << " s, " << virtual_s * ns << " ns/call (count "
            << v->count << ")\n";
  std::cout << "CRTP:    " << crtp_s << " s, " << crtp_s * ns << " ns/call (count " << c.count
            << ")\n";
#ifndef NDEBUG
  std::cout << "note: unoptimized build; rerun with PRESET=release for real numbers\n";
#endif
}
