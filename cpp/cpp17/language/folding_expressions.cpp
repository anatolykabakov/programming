#include <iostream>

template <typename... Args>
bool logical_and(Args... args) {
    // binary folding
    return (... && args);
}

template <typename... Args>
auto sum(Args... args) {
    // unary folding
    return (... + args);
}

int main() {
    std::cout << sum(1.0, 2.0, 3.0) << std::endl;
    std::cout << logical_and(true, false, true) << std::endl;
    std::cout << logical_and(true, true, true) << std::endl;

    return 0;
}
