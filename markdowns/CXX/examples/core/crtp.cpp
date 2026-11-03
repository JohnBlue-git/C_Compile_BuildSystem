// CRTP static interface: Animal<Derived> calls Derived::make_sound()
// without virtual functions. A concept checks the interface at compile time.
#include <concepts>
#include <iostream>

template <typename T>
concept IsAnimal = requires(T v) {
  { v.make_sound() } -> std::same_as<void>;
};

template <typename Derived>
class Animal {
public:
  // The check lives in the constructor, not at class scope: when
  // Animal<Dog> is instantiated as a base, Dog is still incomplete.
  Animal() {
    static_assert(IsAnimal<Derived>,
                  "\n\n[CRTP Error]: Your Derived class is missing 'void make_sound()'.\n"
                  "Remember to make it private and add 'friend class Animal<Derived>;'\n");
  }

  // The public interface. Static dispatch: no vtable.
  void speak() { static_cast<Derived*>(this)->make_sound(); }
};

class Dog : public Animal<Dog> {
private:
  friend class Animal<Dog>;  // lets the base call the private hook
  void make_sound() { std::cout << "Woof! (static dispatch)\n"; }
};

class Cat : public Animal<Cat> {
private:
  friend class Animal<Cat>;
  void make_sound() { std::cout << "Meow... (static dispatch)\n"; }
};

// Missing make_sound(): creating an object triggers the static_assert.
class SilentFish : public Animal<SilentFish> {};

// Generic code over the static interface: one instantiation per type.
template <typename T>
void make_it_speak(Animal<T>& animal) {
  animal.speak();
}

int main() {
  Dog d;
  Cat c;
  d.speak();
  c.speak();
  make_it_speak(d);

  // Uncomment to see the static_assert message at compile time:
  // SilentFish f;
}
