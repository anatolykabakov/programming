#include <iostream>
#include <set>

struct Base {
    Base() { std::cout << "ctor Base" << std::endl; }
    void foo() { std::cout << "Base::foo" << std::endl; }
};

struct Derived1 : Base {
    Derived1() { std::cout << "ctor Derived1" << std::endl; }
    void foo() { std::cout << "Derived1::foo" << std::endl; }
};

int main() {
    Base b;
    b.foo();
    Derived1 d;
    d.foo();

    Base e = d;
    e.foo();

    return 0;
}
