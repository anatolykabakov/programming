#include <thread>
#include <iostream>

class thread_guard {
public:
    explicit thread_guard(std::thread& t): t_(t) {}

    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;

    ~thread_guard() {
        if (t_.joinable()) {
            t_.join();
        }
    }
private:
    std::thread& t_;
};

int main() {
    std::thread t([](){
        for (int i =0; i < 10000; ++i) {
            std::cout << i << " ";
        }
    });
    thread_guard g(t);
    return 0;
}