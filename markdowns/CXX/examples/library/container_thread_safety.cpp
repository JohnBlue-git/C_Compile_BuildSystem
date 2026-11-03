// Standard containers do not lock. Concurrent modification needs a mutex.
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
    std::vector<int> numbers;
    int counter = 0;
    std::mutex mtx;  // protects both numbers and counter

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&] {
            std::lock_guard<std::mutex> lock(mtx);
            numbers.push_back(1);  // may reallocate: must not race with anything
            ++counter;             // plain int: unsynchronized ++ is a data race
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "numbers.size() = " << numbers.size() << '\n';  // 100
    std::cout << "counter        = " << counter << '\n';         // 100

    // Different threads may write *different* elements without a lock,
    // as long as nobody changes the container's structure meanwhile.
    std::vector<int> slots(4, 0);
    std::vector<std::thread> writers;
    for (int i = 0; i < 4; ++i) {
        writers.emplace_back([&slots, i] { slots[i] = i * 10; });
    }
    for (auto& t : writers) {
        t.join();
    }
    for (int v : slots) {
        std::cout << v << ' ';
    }
    std::cout << '\n';
}
