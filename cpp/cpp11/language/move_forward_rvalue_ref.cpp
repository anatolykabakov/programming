#include <iostream>

void f(int &x) { std::cout << "f(int&)" << std::endl; }
void f(int &&x) { std::cout << "f(int&&)" << std::endl; }
void f(auto &&arg) { std::cout << "f(auto&&)" << std::endl; }

// template<typename T>
// void f(T&& arg) {std::cout << "f(T&& arg)" << std::endl;}

int main() {
    // Rvalue references
    int x = 0;    // is a lvalue
    int &xl = x;  // is lvalue reference
    // int&& xr = x; // error
    int &&xr2 = 0;  // is lvalue

    f(x);
    f(xl);
    f(xr2);
    f(3);

    // Move semantics
    f(std::move(x));
    f(std::move(xr2));

    /**
      Forwarding references
      T& & becomes T&
      T& && becomes T&
      T&& & becomes T&
      T&& && becomes T&&
    */

    auto &&al = x;  // is an lvalue of type int&
    auto &&ar = 0;  // is an lvalue of type int&&

    f(0);  // T is int, deduces as f(int &&) => f(int&&)
    f(x);  // T is int&, deduces as f(int& &&) => f(int&)

    int &y = x;
    f(y);  // T is int&, deduces as f(int& &&) => f(int&)

    int &&z = 0;      // NOTE: `z` is an lvalue with type `int&&`.
    f(z);             // T is int&, deduces as f(int& &&) => f(int&)
    f(std::move(z));  // T is int, deduces as f(int &&) => f(int&&)

    return 0;
}
