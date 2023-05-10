#include <chrono>
#include <iostream>
#include <limits>
#include <list>
#include <mutex>
#include <thread>

int main() {
    auto foo = [&](std::mutex &m1, std::mutex &m2) {
        std::cout << "lock" << std::endl;
        std::lock(m1, m2);
        std::cout << "do something" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m1.unlock();
        m2.unlock();
        std::cout << "unlock" << std::endl;
    };

    std::mutex m1;
    std::mutex m2;
    std::thread t1(foo, std::ref(m1), std::ref(m2));
    std::thread t2(foo, std::ref(m2), std::ref(m1));
    t1.join();
    t2.join();

    return 0;
}
