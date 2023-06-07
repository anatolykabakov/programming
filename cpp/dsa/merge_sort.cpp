
#include <cmath>
#include <iostream>
#include <vector>

void merge(std::vector<int>& array, int l, int m, int r)
{
  std::vector<int> left;
  for (int i = l; i < m + 1; ++i) {
    left.push_back(array[i]);
  }
  std::vector<int> right;
  for (int i = m + 1; i <= r; ++i) {
    right.push_back(array[i]);
  }

  int k = l;
  int i = 0;
  int j = 0;

  while (i < left.size() && j < right.size()) {
    if (left[i] <= right[j]) {
      array[k] = left[i];
      ++i;
    } else {
      array[k] = right[j];
      ++j;
    }
    ++k;
  }

  while (i < left.size()) {
    array[k] = left[i];
    ++i;
    ++k;
  }
  while (j < right.size()) {
    array[k] = right[j];
    ++j;
    ++k;
  }
}

// Time O(nlogn) Space O(n)
void merge_sort(std::vector<int>& array, int l, int r)
{
  if (r - l + 1 <= 1) {
    return;
  }

  int mid = l + (r - l) / 2;

  merge_sort(array, l, mid);

  merge_sort(array, mid + 1, r);

  merge(array, l, mid, r);
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
  merge_sort(array, 0, array.size() - 1);
  std::cout << array << std::endl;
  return 0;
}
