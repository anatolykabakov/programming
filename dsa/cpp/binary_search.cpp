
#include <cmath>
#include <iostream>
#include <vector>

int binary_search(const std::vector<int>& array, int target, int start, int end)
{
  while (start <= end) {
    int middle = start + (end - start) / 2;

    if (array[middle] == target) {
      return middle;
    }

    if (target < array[middle]) {
      end = middle - 1;
    } else {
      start = middle + 1;
    }
  }
  return -1;
}

int binary_search_resursive(const std::vector<int>& array, int target, int start, int end)
{
  if (start > end || end < 0) {
    return -1;
  }

  int middle = start + (end - start) / 2;

  if (array[middle] == target) {
    return middle;
  }
  if (target < array[middle]) {
    return binary_search_resursive(array, target, start, middle - 1);
  } else {
    return binary_search_resursive(array, target, middle + 1, end);
  }
}

int main()
{
  std::vector<int> array = {0, 10, 20, 30, 40, 50};

  std::cout << binary_search(array, 10, 0, array.size() - 1) << std::endl;
  std::cout << binary_search_resursive(array, 10, 0, array.size() - 1) << std::endl;

  std::cout << binary_search(array, 100, 0, array.size() - 1) << std::endl;
  std::cout << binary_search_resursive(array, 100, 0, array.size() - 1) << std::endl;

  std::cout << binary_search(array, -100, 0, array.size() - 1) << std::endl;
  std::cout << binary_search_resursive(array, -100, 0, array.size() - 1) << std::endl;

  return 0;
}
