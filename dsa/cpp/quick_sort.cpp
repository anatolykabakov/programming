#include <iostream>
#include <vector>

int partition(std::vector<int>& array, int l, int r)
{
  int left = l;

  for (int i = l; i < r; ++i) {
    if (array[i] < array[r]) {
      std::swap(array[i], array[left]);
      left += 1;
    }
  }

  std::swap(array[left], array[r]);
  return left;
}

// Time O(nlogn), Space O(1)
void quick_sort(std::vector<int>& array, int l, int r)
{
  if (r - l + 1 <= 1) {
    return;
  }
  // partition the array
  int pivot_index = partition(array, l, r);
  // sorting left part
  quick_sort(array, l, pivot_index - 1);
  // sorting right part
  quick_sort(array, pivot_index + 1, r);
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
  quick_sort(array, 0, array.size() - 1);
  std::cout << array << std::endl;

  return 0;
}
