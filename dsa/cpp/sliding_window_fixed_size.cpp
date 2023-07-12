// Q: Given an array, return true if there are two elements within a window of size k that are equal.
#include <iostream>
#include <vector>
#include <unordered_set>

// Time O(N^N)
bool brute_force(const std::vector<int>& arr, int k)
{
  for (std::size_t l = 0; l < arr.size(); ++l) {
    for (std::size_t r = l + 1; r < std::min(arr.size(), l + k); ++r) {
      if (arr[l] == arr[r]) {
        return true;
      }
    }
  }
  return false;
}

// Time O(N)
bool sliding_window(const std::vector<int>& arr, int k)
{
  std::size_t l = 0;
  std::unordered_set<int> s;

  for (std::size_t r = 0; r < arr.size(); ++r) {
    if (s.count(arr[r]) == 1) {
      return true;
    }
    s.insert(arr[r]);
    if (r - l + 1 > k) {
      s.erase(arr[l]);
      l += 1;
    }
  }

  return false;
}

int main()
{
  std::vector<int> with_dublicates = {1, 2, 3, 2, 3, 3};
  int k = 3;
  std::cout << brute_force(with_dublicates, k) << std::endl;
  std::cout << sliding_window(with_dublicates, k) << std::endl;

  std::vector<int> without_dublicates = {1, 2, 3, 4, 5, 6};
  std::cout << brute_force(without_dublicates, k) << std::endl;
  std::cout << sliding_window(without_dublicates, k) << std::endl;
  return EXIT_SUCCESS;
}
