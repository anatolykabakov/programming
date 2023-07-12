
#include <cmath>
#include <iostream>
#include <vector>

// Time O(n^2) Space O(n)
void insertion_sort(std::vector<int>& array)
{
  for (int i = 1; i < (int)array.size(); ++i) {
    int j = i - 1;
    while (j >= 0 && array[j + 1] < array[j]) {
      int tmp = array[j + 1];
      array[j + 1] = array[j];
      array[j] = tmp;
      j -= 1;
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
  insertion_sort(array);
  std::cout << array << std::endl;

  return 0;
}
