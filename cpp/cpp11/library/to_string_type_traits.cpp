#include <iostream>

int main() {
    std::to_string(1.2);  // == "1.2"
    std::to_string(123);  // == "123"

    static_assert(std::is_integral<int>::value);
    static_assert(std::is_same<int, int>::value);
    static_assert(std::is_same<std::conditional<true, int, double>::type, int>::value);
    return 0;
}
