#include <iostream>
#include <set>

struct Foo {
    int i;
};

struct A {
    double c;
    int b;
    bool a;
};

struct EmptyStruct {};

struct EmptyClass {};

int main() {
    std::cout << "bool " << sizeof(true) << std::endl;
    std::cout << "double " << sizeof(0.0) << std::endl;
    std::cout << "uint " << sizeof(1u) << std::endl;
    std::cout << "int " << sizeof(1) << std::endl;
    std::cout << "float " << sizeof(0.0f) << std::endl;
    std::cout << "void* " << sizeof(void *) << std::endl;
    std::cout << "A(double, int, bool) " << sizeof(A) << std::endl;
    std::cout << "EmptyStruct " << sizeof(EmptyStruct) << std::endl;
    std::cout << "EmptyClass " << sizeof(EmptyClass) << std::endl;
    return 0;
}
