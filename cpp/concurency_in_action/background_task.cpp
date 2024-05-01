#include <thread>
#include <iostream>

class background_task {
public:
  void operator() () {
    do_something();
  }

  void do_something() {
    std::cout << __func__ << std::endl;
  }
};

struct func {
    int& i_;
    func(int& i): i_{i} {}

    void operator() () {
        for (int i = 0; i < 100000; ++i) {
            std::cout << i_ + i << std::endl;
        }
    }
};

void oops() {
    int some_local_state = 12340;
    func my_func(some_local_state);
    std::thread t(my_func);
    t.detach();

}

int main() {
    background_task task;
    std::thread t1(task);
    t1.join();

    std::thread t2{background_task()};
    t2.join();
    
    oops();
    return 0;

}