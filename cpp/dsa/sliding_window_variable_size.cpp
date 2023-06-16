#include <iostream>
#include <vector>
#include <unordered_set>

// Time O(N)
int longest_subarray_length(const std::vector<int>& arr)
{
  int l = 0;
  int length = 1;

  for (int r = 1; r < arr.size(); ++r) {
    if (arr[r] != arr[l]) {
      l = r;
    }
    length = std::max(length, r - l + 1);
  }

  return length;
}

// Time O(N)
int shortest_subarray_length(const std::vector<int>& arr, int target)
{
  int l = 0;
  int length = 1000000;
  int sum = 0;

  for (int r = 0; r < arr.size(); ++r) {
    sum += arr[r];
    while (sum >= target) {
      length = std::min(length, r - l + 1);
      sum -= arr[l];
      l += 1;
    }
  }

  return length == 1000000 ? 0 : length;
}

int main()
{
  // Q: Find the length of the longest subarray, with the same value in each position.
  std::vector<int> arr = {4, 2, 2, 3, 3, 3};
  std::cout << longest_subarray_length(arr) << std::endl;

  // Q: Find the minimum length subarray, where the sum is greater than or equal to the target. Assume all values are
  // positive.
  arr = {2, 3, 1, 2, 4, 3};
  std::cout << shortest_subarray_length(arr, 6) << std::endl;

  return EXIT_SUCCESS;
}
