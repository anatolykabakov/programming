
#include <cmath>
#include <iostream>
#include <vector>

void bubble_sort(std::vector<int>& array)
{
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

std::ostream& operator<<(std::ostream& os, const std::vector<int>& arr)
{
  for (const auto& item : arr) {
    std::cout << item << " ";
  }
  std::cout << std::endl;
  return os;
}

int main()
{
  std::vector<int> array = {0, 3, 2, 5, 6, 8, 1, 9, 4, 2, 1, 2, 9, 6, 4, 1, 7, -1, -5, 23, 6, 2, 35, 6, 3, 32};
  std::cout << array << std::endl;
  bubble_sort(array);
  std::cout << array << std::endl;

  return 0;
}
