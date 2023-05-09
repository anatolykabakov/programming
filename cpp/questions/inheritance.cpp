#include <iostream>

class Foo {
public:
    Foo() { std::cout << "ctor Foo " << std::endl; }
    Foo(const Foo &) { std::cout << "copy ctor Foo " << std::endl; }
    Foo &operator=(const Foo &) { std::cout << "copy operator Foo " << std::endl; }
    Foo(Foo &&) { std::cout << "move ctor Foo " << std::endl; }
    Foo &operator=(Foo &&) { std::cout << "move operator Foo " << std::endl; }
    virtual void f() { std::cout << "A" << std::endl; }
    ~Foo() { std::cout << "dtor Foo " << std::endl; }
    // virtual ~Foo() { std::cout << "dtor Foo " << std::endl;}
};

class Bar : public Foo {
public:
    Bar() { std::cout << "ctor Bar " << std::endl; }
    virtual void f() { std::cout << "B" << std::endl; }
    ~Bar() { std::cout << "dtor Bar " << std::endl; }
};

void foo(Foo obj) { obj.f(); }

void bar(Foo &obj) { obj.f(); }

int main() {
    Foo f;
    foo(f);  // print A
    bar(f);  // print A

    Bar b;
    foo(b);  // print A
    bar(b);  // print B

    Foo *obj = new Bar();
    delete obj;  // dtor Foo will be called instead dtor Bar. Needed to write a virtual
                 // dtor for Foo, to call dtor Bar
    return 0;
}
