#include <algorithm>
#include <iostream>
#include <vector>

template <typename T>
int CountTwos(const T &container) {
    return std::count_if(std::begin(container), std::end(container),
                         [](int item) { return item == 2; });
}

int main() {
    std::vector<int> vec = {2, 2, 43, 435, 4543, 534};
    int arr[8] = {2, 43, 45, 435, 32, 32, 32, 32};
    auto a = CountTwos(vec);  // 2
    auto b = CountTwos(arr);  // 1
}
