#include <string>
#include <iostream>
#include <thread>
#include <cassert>

void bar(std::string & data) {
    data += "bar";
}

class Bar {
public:
    void do_lengthy_work() {
        for (int i=0; i<1000; ++i) {
            std::cout << i << " ";
        }
    }
};

void do_work(std::unique_ptr<Bar> ptr) {
    ptr->do_lengthy_work();
}

int main() {
    auto foo = [](std::string &data) {
        data += "foo";
    };
    std::string s("hello");
    std::thread t1(foo, std::ref(s));
    t1.join();
    std::cout << s << std::endl;
    
    Bar bar;
    std::thread t2(&Bar::do_lengthy_work, &bar);
    t2.join();

    auto p1 = std::make_unique<Bar>();
    std::thread t3(do_work, std::move(p1));
    t3.join();
    assert(p1 == nullptr);
    return 0;
}