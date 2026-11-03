// unique_ptr / shared_ptr / weak_ptr basics and enable_shared_from_this.
#include <iostream>
#include <memory>

class Object : public std::enable_shared_from_this<Object> {
public:
  explicit Object(int id) : id_(id) { std::cout << "Object " << id_ << " constructed\n"; }
  ~Object() { std::cout << "Object " << id_ << " destroyed\n"; }
  Object(const Object&) = delete;
  Object& operator=(const Object&) = delete;

  void hello() const { std::cout << "hello from Object " << id_ << '\n'; }

  // Needs an existing shared_ptr owner. Since C++17 it throws
  // std::bad_weak_ptr otherwise (before C++17: undefined behavior).
  std::shared_ptr<const Object> self() const { return shared_from_this(); }

private:
  int id_;
};

int main() {
  std::cout << "--- unique_ptr ---\n";
  {
    auto u1 = std::make_unique<Object>(1);
    auto u2 = std::move(u1);  // ownership moves; u1 becomes null
    std::cout << "u1 is " << (u1 ? "non-null" : "null") << " after move\n";
    u2->hello();

    try {
      u2->self();  // owned by unique_ptr, not shared_ptr
    } catch (const std::bad_weak_ptr& e) {
      std::cout << "shared_from_this without shared owner: " << e.what() << '\n';
    }
  }  // Object 1 destroyed here

  std::cout << "--- shared_ptr / weak_ptr ---\n";
  std::weak_ptr<Object> weak;
  {
    auto s1 = std::make_shared<Object>(2);
    auto s2 = s1;  // copy: use_count + 1
    weak = s1;     // weak_ptr does not change use_count
    std::cout << "use_count = " << s1.use_count() << '\n';

    auto s3 = s1->self();  // shares the same control block
    std::cout << "use_count after shared_from_this = " << s1.use_count() << '\n';

    if (auto locked = weak.lock()) {
      locked->hello();
    }
  }  // last owner gone: Object 2 destroyed here

  if (auto locked = weak.lock()) {
    locked->hello();
  } else {
    std::cout << "weak.lock() returned null: object already destroyed\n";
  }
  std::cout << "weak.expired() = " << std::boolalpha << weak.expired() << '\n';
}
