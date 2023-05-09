#include <iostream>

void f(int &&a) { std::cout << "int &&a" << std::endl; }

template <typename T>
T &&forward(T &obj) {
    return static_cast<T &&>(obj);
}

template <typename T>
void g(T &&a) {
    f(forward<T>(a));
}

int main() {
    int a = 42;
    // f(a);
    g(42);
    return 0;
}
