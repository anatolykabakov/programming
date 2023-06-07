
#include <cmath>
#include <iostream>
#include <vector>

// Time O(n) Space O(1)
void bucket_sort(std::vector<int>& array)
{
  std::vector<int> range = {0, 0, 0};

  for (const auto& el : array) {
    range[el]++;
  }

  int k = 0;
  for (int i = 0; i < array.size(); ++i) {
    for (int j = 0; j < range[i]; ++j) {
      array[k] = i;
      ++k;
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
  std::vector<int> array = {2, 1, 2, 0, 0, 2};
  std::cout << array << std::endl;
  bucket_sort(array);
  std::cout << array << std::endl;

  return 0;
}
