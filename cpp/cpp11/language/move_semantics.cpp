#include <iostream>

struct A {
    std::string s;
    A() : s{"test"} { std::cout << "A()" << std::endl; }
    A(const A &o) : s{o.s} { std::cout << "A(const A& o)" << std::endl; }
    A(A &&o) : s{std::move(o.s)} { std::cout << "A(A&& o)" << std::endl; }
    A &operator=(A &&o) {
        std::cout << "A& operator=(A&& o)" << std::endl;
        s = std::move(o.s);
        return *this;
    }
};

A f(A a) { return a; }

int main() {
    A a1 = f(A{});         // move-constructed from rvalue temporary
    A a2 = std::move(a1);  // move-constructed using std::move
    A a3 = A{};
    a2 = std::move(a3);  // move-assignment using std::move
    a1 = f(A{});         // move-assignment from rvalue temporary
}
