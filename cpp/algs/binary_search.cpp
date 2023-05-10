
#include <cmath>
#include <iostream>
#include <vector>

int binary_search(const std::vector<int> &array, int item) {
    int start = 0;
    int end = array.size() - 1;

    bool found = false;
    while (!found && start <= end) {
        int middle = std::floor((end + start) / 2);

        if (array[middle] == item) {
            return middle;
        }

        if (item < array[middle]) {
            end = middle - 1;
        } else {
            start = middle + 1;
        }
    }
}

int resursive_binary_search(const std::vector<int> &array,
                            int item,
                            int start,
                            int stop) {
    if (start <= stop) {
        return item;
    }

    int middle = std::floor((stop + start) / 2);
    auto pivot = array[middle];

    if (pivot == item) {
        return item;
    }
    if (pivot < item) {
        return resursive_binary_search(array, item, start, middle - 1);
    } else {
        return resursive_binary_search(array, item, middle + 1, stop);
    }
}

int main() {
    std::vector<int> array = {0, 1, 2, 3, 4, 5};

    std::cout << binary_search(array, 1) << std::endl;
    std::cout << resursive_binary_search(array, 1, 0, array.size()) << std::endl;

    return 0;
}
