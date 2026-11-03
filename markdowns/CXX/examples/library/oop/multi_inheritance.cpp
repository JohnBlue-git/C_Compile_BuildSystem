// Multiple inheritance, the diamond problem, and virtual inheritance.
#include <iostream>

class Grand {
public:
    Grand() : x_(0) { std::cout << "  Grand()\n"; }
    explicit Grand(int v) : x_(v) { std::cout << "  Grand(" << v << ")\n"; }
    virtual ~Grand() = default;
    int x() const { return x_; }

private:
    int x_;
};

// Without virtual inheritance: Kid contains TWO Grand subobjects.
class Dad : public Grand { public: Dad() : Grand(30) { std::cout << "  Dad()\n"; } };
class Mom : public Grand { public: Mom() : Grand(33) { std::cout << "  Mom()\n"; } };
class Kid : public Dad, public Mom {
public:
    // Bases are constructed in the order they are declared above (Dad, Mom).
    // Writing `: Mom(), Dad()` would not change that (GCC warns: -Wreorder).
    Kid() : Dad(), Mom() { std::cout << "  Kid()\n"; }
};

// With virtual inheritance: ONE shared Grand subobject.
class DadV : virtual public Grand { public: DadV() : Grand(300) { std::cout << "  DadV()\n"; } };
class MomV : virtual public Grand { public: MomV() : Grand(303) { std::cout << "  MomV()\n"; } };
class KidV : public DadV, public MomV {
public:
    // The most-derived class constructs the virtual base. DadV's and MomV's
    // Grand(...) initializers are ignored here.
    KidV() { std::cout << "  KidV()\n"; }                   // uses Grand()
    explicit KidV(int v) : Grand(v) { std::cout << "  KidV(int)\n"; }
};

int main() {
    std::cout << "Kid (no virtual inheritance):\n";
    Kid k;
    // k.x();  // error: ambiguous, there are two Grand subobjects
    std::cout << "  Dad's Grand x = " << k.Dad::x() << ", Mom's Grand x = " << k.Mom::x() << '\n';
    // Grand* g = &k;  // error: ambiguous base

    std::cout << "\nKidV (virtual inheritance), default:\n";
    KidV kv;
    std::cout << "  x = " << kv.x() << '\n';  // 0: Grand() was used

    std::cout << "\nKidV(333) through a Grand*:\n";
    Grand* p = new KidV(333);  // OK: only one Grand subobject
    std::cout << "  x = " << p->x() << '\n';  // 333
    delete p;  // virtual ~Grand() makes this safe
}
