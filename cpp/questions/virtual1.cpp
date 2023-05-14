#include <iostream>
class A {
public:
    virtual void Print() { std::cout << "A" << std::endl; };
};

class B : public A {
public:
    void Print() { std::cout << "B" << std::endl; };
};

template <typename T>
void TPrint(T obj) {
    obj.Print();
};

template <typename T>
void RPrint(T &obj) {
    obj.Print();
};

int main() {
    B *b = new B();
    b->Print();  // B

    A &a = *b;
    TPrint(a);  // A
    RPrint(a);  // B
    a.Print();  // B
    return 0;
}
