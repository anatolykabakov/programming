#include <chrono>
#include <iostream>
#include <tuple>

void foo() {
    int i = 0;
    while (i < 100000) {
        ++i;
    }
}

int main() {
    // std::chrono
    auto tic = std::chrono::high_resolution_clock::now();
    foo();
    auto toc = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count()
              << " ms" << std::endl;

    // Tuples

    const auto playerProfile = std::make_tuple(51, "Frans Nielsen", "NYI");
    std::cout << std::get<0>(playerProfile) << " " << std::get<1>(playerProfile) << " "
              << std::get<2>(playerProfile) << std::endl;  // "NYI"

    // With tuples...
    std::string playerName;
    std::tie(std::ignore, playerName, std::ignore) =
        std::make_tuple(91, "John Tavares", "NYI");

    // With pairs...
    std::string yes, no;
    std::tie(yes, no) = std::make_pair("yes", "no");
    return 0;
}
