#include <iostream>

template <typename T>
struct foo {
    foo(T arg) { std::cout << "__func__" << std::endl; }
};

template <>
struct foo<bool> {
    foo(bool arg) { std::cout << "specialization __func__" << std::endl; }
};

template <typename T>
void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

struct A {
    double c;
    int b;
    bool a;
};

int main() {
    foo<int> a(0);
    foo<double> b(0.0);
    foo<bool> c(true);

    auto p = &swap<int>;
    auto q = &swap<char>;

    std::cout << "bool " << sizeof(true) << std::endl;
    std::cout << "double " << sizeof(0.0) << std::endl;
    std::cout << "int " << sizeof(1) << std::endl;
    std::cout << "float " << sizeof(0.0f) << std::endl;
    std::cout << "void* " << sizeof(void *) << std::endl;
    std::cout << "A " << sizeof(A) << std::endl;
    return 0;
}
