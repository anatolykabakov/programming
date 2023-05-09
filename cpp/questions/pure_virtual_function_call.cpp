#include <iostream>
#include <set>

struct A {
    A() {
        std::cout << __func__ << std::endl;
        bar();
    }
    void bar() {
        std::cout << "A::bar" << std::endl;
        foo();  // pure virtual function called
    }
    virtual void foo() = 0;
};

struct B : A {
    void foo() { std::cout << "B::foo" << std::endl; }
};

int main() {
    B *b = new B();
    return 0;
}
