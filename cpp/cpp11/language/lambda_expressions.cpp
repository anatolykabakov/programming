#include <iostream>

int main() {
    int x = 1;
    // capture by reference
    auto capture_by_reference = [&]() { return ++x; };
    auto capture_by_value = [=]() { return x + 1; };
    auto capture_by_value_with_arg = [=](int y) { return x + y; };
    // auto f2 = [x] { x = 2; }; // ERROR: the lambda can only perform const-operations on
    // the captured value vs.
    auto f3 = [x]() mutable {
        x = 2;
    };  // OK: the lambda can perform any operations on the captured value

    std::cout << capture_by_reference() << " " << x << std::endl;
    std::cout << capture_by_value() << " " << x << std::endl;
    std::cout << capture_by_value_with_arg(10) << std::endl;
    f3();
    std::cout << x << std::endl;
    return 0;
}
