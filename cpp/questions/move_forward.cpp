#include <iostream>
#include <utility>

int foo(int &arg) { std::cout << "foo(int&)" << std::endl; }
int foo(int &&arg) { std::cout << "foo(int&&)" << std::endl; }

template <typename T>
int a(T &&x) {
    foo(x);
}
template <typename T>
int b(T &&x) {
    foo(std::move(x));
}
template <typename T>
int c(T &&x) {
    foo(std::forward<T>(x));
}

int main() {
    int var = 1;
    a(var);
    b(var);
    c(var);
    a(1);
    b(1);
    c(1);
    return 0;
}
