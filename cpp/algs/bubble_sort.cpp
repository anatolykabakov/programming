
#include <cmath>
#include <iostream>
#include <vector>

void bubble_sort(std::vector<int> &array) {
    for (int i = 0; i < array.size(); ++i) {
        for (int j = 0; j < array.size(); ++j) {
            if (array[j] > array[j + 1]) {
                int tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }
    }
}

int main() {
    std::vector<int> array = {0, 3, 2, 1, 4, 5};

    bubble_sort(array);
    for (const auto &item : array) {
        std::cout << item << std::endl;
    }

    return 0;
}
