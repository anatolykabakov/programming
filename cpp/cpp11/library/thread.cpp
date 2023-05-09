#include <future>
#include <iostream>
#include <thread>
#include <vector>

void foo(int begin, int end) {
    while (begin <= end) {
        std::cout << begin << std::endl;
        ++begin;
    }
}

int bar(int i) { return i + 1; }

int main() {
    int a = 0;
    std::thread t1{[&]() { ++a; }};
    std::thread t2([&]() { a++; });
    std::thread t3([&]() { a = a + 1; });
    std::cout << a << std::endl;
    t1.join();
    t2.join();
    t3.join();

    std::vector<std::thread> threadsVector;
    threadsVector.emplace_back([]() {
        // Lambda function that will be invoked
    });
    threadsVector.emplace_back(foo, 0, 10);   // thread will run foo with args 0, 10
    threadsVector.emplace_back(foo, 10, 20);  // thread will run foo with args 10, 20
    threadsVector.emplace_back(foo, 20, 30);  // thread will run foo with args 10, 20
    for (auto &thread : threadsVector) {
        thread.join();  // Wait for threads to finish
    }

    std::future<int> handle =
        std::async(std::launch::async, bar, 1);  // create an async task
    auto result = handle.get();                  // wait for the result
    std::cout << result << std::endl;
    return 0;
}
