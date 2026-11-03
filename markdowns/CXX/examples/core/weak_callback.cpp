// A stored callback that captures weak_ptr does not keep its object alive,
// and can detect that the object is gone before calling into it.
#include <functional>
#include <iostream>
#include <memory>

class Session : public std::enable_shared_from_this<Session> {
public:
  ~Session() { std::cout << "Session destroyed\n"; }

  // Safe: the callback checks the object is still alive.
  std::function<void()> make_weak_callback() {
    std::weak_ptr<Session> weak_self = shared_from_this();
    return [weak_self] {
      auto self = weak_self.lock();
      if (!self) {
        std::cout << "weak callback: object gone, skipping\n";
        return;
      }
      self->do_something();
    };
  }

  // Keeps the object alive as long as the callback exists.
  std::function<void()> make_keep_alive_callback() {
    return [self = shared_from_this()] { self->do_something(); };
  }

  void do_something() const { std::cout << "doing something\n"; }
};

int main() {
  std::function<void()> weak_cb;
  std::function<void()> strong_cb;

  {
    auto session = std::make_shared<Session>();
    weak_cb = session->make_weak_callback();
    weak_cb();  // object alive: runs
  }  // last shared_ptr gone: "Session destroyed"

  std::cout << "calling weak callback after the owner left scope:\n";
  weak_cb();

  {
    auto session = std::make_shared<Session>();
    strong_cb = session->make_keep_alive_callback();
  }  // NOT destroyed: strong_cb still owns a shared_ptr

  std::cout << "calling keep-alive callback after the owner left scope:\n";
  strong_cb();
  strong_cb = nullptr;  // releases the captured shared_ptr: "Session destroyed"
  std::cout << "end of main\n";
}
