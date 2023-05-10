#include <iostream>
#include <variant>

int main() {
    std::variant<int, double> v{12};  // int
    std::cout << std::get<int>(v) << std::endl;
    std::cout << std::get<0>(v) << std::endl;
    v = 12.0;

    std::get<double>(v);  // == 12.0
    std::get<1>(v);       // == 12.0
    return 0;
}
