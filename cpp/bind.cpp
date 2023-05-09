#include <functional>
#include <iostream>

void print(const std::string &a, int b) { std::cout << a << " " << b << std::endl; }

void f(int n1, int n2, int n3, int n4, int n5) {
    std::cout << n1 << ' ' << n2 << ' ' << n3 << ' ' << n4 << ' ' << n5 << '\n';
}

int main() {
    int n = 7;
    auto f1 = std::bind(f, std::placeholders::_1, 42, std::placeholders::_2, n, n);
    n = 10;
    f1(1, 2, 1001);

    std::string s = "hello, world";

    auto print_b = std::bind(print, s, std::placeholders::_1);

    print_b(2);

    return 0;
}
