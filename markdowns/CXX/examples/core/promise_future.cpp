// std::promise / std::future: a one-shot channel between threads.
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

void async_task(std::promise<int> promise) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  promise.set_value(42);  // wakes up the thread blocked in future.get()
}

int main() {
  std::promise<int> promise;
  std::future<int> future = promise.get_future();

  // promise is move-only, so it must be moved into the thread.
  std::thread worker(async_task, std::move(promise));

  std::cout << "waiting for the result...\n";
  std::cout << "result: " << future.get() << '\n';  // blocks until set_value
  worker.join();

  // A promise destroyed without a value: get() throws broken_promise.
  std::future<int> orphan;
  {
    std::promise<int> forgotten;
    orphan = forgotten.get_future();
  }
  try {
    orphan.get();
  } catch (const std::future_error& e) {
    std::cout << "future_error: " << e.code().message() << '\n';
  }
}
