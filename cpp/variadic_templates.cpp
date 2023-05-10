#include <iostream>

template <typename... U>
struct tuple {};

void g(int a, float b, char c) { std::cout << a << " " << b << " " << c << std::endl; }

struct agg {
    int a;
    float b;
    char c;
};

template <typename... V>
void f(V... args) {
    g(args...);
}

template <class Arg>
Arg sum(Arg arg) {
    std::cout << "Arg sum(Arg arg) " << arg << std::endl;
    return arg;
}

template <class First, class... Other>
auto sum(First first, Other... other) {
    std::cout << "auto sum(First first, Other... other) " << first << std::endl;
    return first + sum(other...);
}

template <typename T>
void print(T arg) {
    std::cout << arg << std::endl;
}

template <typename FirstArg, typename... T>
void print(FirstArg arg, T... other) {
    std::cout << arg << std::endl;
    print(other...);
}

int main() {
    f<int, float, char>(1, 1.f, '1');
    std::cout << sum(0, 1, 2, 3) << std::endl;

    print(0, '1', "string");
}
