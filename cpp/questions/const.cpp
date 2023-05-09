#include <iostream>
#include <memory>

class Foo {
public:
    Foo() {}
    void bar() const {
        std::cout << __func__ << std::endl;
        // i = 2; // error
    }
    void baz() {
        std::cout << __func__ << std::endl;
        i = 2;
    }
    ~Foo() {}

private:
    int i = 0;
};

void bar(const Foo &foo) {
    foo.bar();
    // foo.baz(); // ERROR
}

int main() {
    const int i = 1;
    // i = 2; // error C3892: 'i' : you cannot assign to a variable that is const
    int j = 4;
    int *const k = new int(2);
    std::cout << *k << std::endl;
    *k = j;  // ok
    std::cout << *k << std::endl;

    // k = &j;

    const int *u = new int(2);
    // *u = j; error
    u = &j;  // ok

    Foo f;
    f.baz();  // ok
    f.bar();

    bar(f);

    return 0;
}
