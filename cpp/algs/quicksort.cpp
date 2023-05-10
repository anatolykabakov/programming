#include <iostream>
#include <vector>

// Time O(nlogn), Space O(n)
std::vector<int> quicksort(const std::vector<int> &array) {
    if (array.size() <= 1) {
        return array;
    }
    int middle = (array.size()) / 2;
    auto pivot = array[middle];
    std::vector<int> greater;
    std::vector<int> less;
    for (const auto &item : array) {
        if (pivot == item) {
            continue;
        }
        if (pivot < item) {
            less.push_back(item);
        } else {
            greater.push_back(item);
        }
    }
    auto less_sorted = quicksort(less);
    auto greater_sorted = quicksort(greater);
    std::vector<int> new_arr;
    for (auto &item : less_sorted) {
        new_arr.push_back(item);
    }
    new_arr.push_back(pivot);
    for (auto &item : greater_sorted) {
        new_arr.push_back(item);
    }
    return new_arr;
}

int main() {
    std::vector<int> arr = {0, 3, 2, 5, 6,  8,  1,  9, 4, 2,  1, 2, 9,
                            6, 4, 1, 7, -1, -5, 23, 6, 2, 35, 6, 3, 32};
    auto sorted = quicksort(arr);
    for (auto &item : sorted) {
        std::cout << item << std::endl;
    }
    return 0;
}
