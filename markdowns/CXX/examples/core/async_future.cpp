// std::async + std::future: results, exceptions, launch policies,
// and the classic trap of a discarded future.
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

long long ms_since(Clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}

void on_task_complete(int result) { std::cout << "task completed with result " << result << '\n'; }

// Runs on another thread and reports through a callback.
void async_function(std::function<void(int)> callback, int x) {
  std::this_thread::sleep_for(100ms);
  callback(x);
}

int main() {
  // 1. Fire a task, keep working, then wait.
  std::future<void> done = std::async(std::launch::async, async_function, on_task_complete, 10);
  std::cout << "main thread keeps working...\n";
  done.get();  // waits; for future<T>, get() returns the T

  // 2. Return a value.
  std::future<int> square = std::async(std::launch::async, [](int x) { return x * x; }, 7);
  std::cout << "7 * 7 = " << square.get() << '\n';

  // 3. An exception thrown in the task is rethrown by get().
  std::future<int> failing =
      std::async(std::launch::async, []() -> int { throw std::runtime_error("task failed"); });
  try {
    failing.get();
  } catch (const std::exception& e) {
    std::cout << "get() rethrew: " << e.what() << '\n';
  }

  // 4. launch::deferred: nothing runs until get()/wait(), then it runs on THIS thread.
  const auto main_id = std::this_thread::get_id();
  auto lazy = std::async(std::launch::deferred,
                         [main_id] { return std::this_thread::get_id() == main_id; });
  std::cout << "deferred task ran on main thread: " << std::boolalpha << lazy.get() << '\n';

  // 5. Trap: the returned future is a temporary. Its destructor waits for
  //    the launch::async task, so this line blocks for the full 150 ms.
  auto start = Clock::now();
  static_cast<void>(std::async(std::launch::async, [] { std::this_thread::sleep_for(150ms); }));
  std::cout << "discarded future: main blocked " << ms_since(start) << " ms\n";

  // Fix: keep the future alive while doing other work.
  start = Clock::now();
  auto kept = std::async(std::launch::async, [] { std::this_thread::sleep_for(150ms); });
  std::cout << "kept future: main free after " << ms_since(start) << " ms\n";
  kept.wait();
  std::cout << "task finished after " << ms_since(start) << " ms\n";
}
