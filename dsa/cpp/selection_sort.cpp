
#include <cmath>
#include <iostream>
#include <vector>

void selection_sort(std::vector<int>& array)
{
  for (int i = 0; i < array.size(); ++i) {
    for (int j = i + 1; j < array.size(); ++j) {
      if (array[j] < array[i]) {
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
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
  selection_sort(array);
  std::cout << array << std::endl;

  return 0;
}
