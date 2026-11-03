// Copy elision: RVO (prvalue, guaranteed since C++17) vs NRVO (optional).
// Built twice: cxx_rvo_nrvo (default) and cxx_rvo_nrvo_no_elide
// (-fno-elide-constructors). Compare the two outputs.
#include <iostream>

class Tracer {
public:
  Tracer() { std::cout << "  Constructor\n"; }
  Tracer(const Tracer&) { std::cout << "  Copy Constructor\n"; }
  Tracer(Tracer&&) noexcept { std::cout << "  Move Constructor\n"; }
  ~Tracer() { std::cout << "  Destructor\n"; }
};

// RVO: returns a prvalue. C++17 guarantees no copy/move, under any flags.
Tracer make_rvo() { return Tracer(); }

// NRVO: returns a named local. Allowed, not required.
Tracer make_nrvo() {
  Tracer obj;
  return obj;
}

// Two candidates: the compiler cannot construct both in the return slot,
// so NRVO usually does not apply. The return is an implicit move, not a copy.
Tracer make_one_of_two(bool first) {
  Tracer a;
  Tracer b;
  if (first) return a;
  return b;
}

// Function parameters are never NRVO candidates: implicit move.
Tracer pass_through(Tracer t) { return t; }

int main() {
  std::cout << "RVO (prvalue):\n";
  { Tracer x = make_rvo(); }

  std::cout << "NRVO (named local):\n";
  { Tracer x = make_nrvo(); }

  std::cout << "two named candidates:\n";
  { Tracer x = make_one_of_two(true); }

  std::cout << "returning a parameter:\n";
  { Tracer x = pass_through(Tracer()); }
}
