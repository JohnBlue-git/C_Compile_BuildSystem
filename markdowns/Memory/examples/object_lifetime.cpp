// object_lifetime.cpp - when C++ objects with each storage duration are
// constructed and destroyed. Read the output top to bottom.
#include <cstdio>
#include <memory>
#include <new>

struct Tracer {
    const char *name;
    explicit Tracer(const char *n) : name(n) { std::printf("  construct %s\n", name); }
    ~Tracer() { std::printf("  destroy   %s\n", name); }
    Tracer(const Tracer &) = delete;
    Tracer &operator=(const Tracer &) = delete;
};

Tracer global{"global (static storage)"}; // before main

Tracer &function_local() {
    static Tracer local{"function-local static"}; // on the first call only
    return local;
}

int main() {
    std::puts("main starts");
    {
        Tracer automatic{"automatic"};
        std::puts("  ...leaving the block");
    } // destroyed here

    Tracer *raw = new Tracer{"new/delete"};
    delete raw; // destroyed only when the owner says so

    {
        auto owned = std::make_unique<Tracer>("unique_ptr");
    } // the owner's scope ends -> delete runs automatically

    // Storage and lifetime are separate: the buffer exists before and after
    // the object that placement new constructs in it.
    alignas(Tracer) unsigned char buffer[sizeof(Tracer)];
    Tracer *placed = new (buffer) Tracer{"placement new"};
    placed->~Tracer(); // must be called by hand; the buffer stays

    function_local();
    function_local(); // no second construction
    std::puts("main returns");
    return 0;
} // after main: static objects are destroyed in reverse order of construction
