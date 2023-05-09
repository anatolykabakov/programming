// template <typename T>
// typename remove_reference<T>::type&& move(T&& arg) {
//   return static_cast<typename remove_reference<T>::type&&>(arg);
// }

// template <typename T>
// T&& forward(typename remove_reference<T>::type& arg) {
//   return static_cast<T&&>(arg);
// }
#include <iostream>
#include <memory>

struct A {
    A() { std::cout << "A()" << std::endl; }
    A(const A &) { std::cout << "A(const A&)" << std::endl; }
    A(A &&) { std::cout << "A(A&&)" << std::endl; }
    A &operator=(A &&) { std::cout << "A& operator=(A&&)" << std::endl; }
    ~A() { std::cout << "~A()" << std::endl; }
};

template <typename T>
A wrapper(T &&arg) {
    return A{std::forward<T>(arg)};
}

int main() {
    std::unique_ptr<A> a{new A()};
    // std::unique_ptr<A> b = a; // error
    std::unique_ptr<A> b = std::move(a);

    wrapper(A{});  // moved
    A c;
    wrapper(c);             // copied
    wrapper(std::move(c));  // moved

    return 0;
}
