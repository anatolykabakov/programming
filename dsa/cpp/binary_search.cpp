
#include <cmath>
#include <iostream>
#include <vector>
#include <cassert>

/**
 * @brief Find the target number in the sorted sequence.
 * If the target number found, the index of target is returned, otherwise -1.
 *
 * @param nums std::vector<int> The sequence of integers. Elements must be distinct and sorted.
 * @param target The number to be found
 * @param l int The begin index of sequence
 * @param r int The end index of sequence
 * @return int The nums's index, if target to be found, otherwise -1.
 * @details Time O(logn) Space O(1)
 */
int binary_search_iterative(const std::vector<int>& nums, int target, int l, int r)
{
  if (nums.empty())
    return -1;

  while (l <= r) {
    int middle = l + (r - l) / 2;

    if (nums[middle] == target) {
      return middle;
    }

    if (target < nums[middle]) {
      r = middle - 1;
    } else {
      l = middle + 1;
    }
  }
  return -1;
}

/**
 * @brief Find the target number in the sorted sequence.
 * If the target number found, the index of target is returned, otherwise -1.
 *
 * @param nums std::vector<int> The sequence of integers. Elements must be distinct and sorted.
 * @param target The number to be found
 * @param l int The begin index of sequence
 * @param r int The end index of sequence
 * @return int The nums's index, if target to be found, otherwise -1.
 * @details Time O(logn) Space O(logn)
 */
int binary_search_resursive(const std::vector<int>& nums, int target, int l, int r)
{
  if (l > r || r < 0 || nums.empty()) {
    return -1;
  }

  int middle = l + (r - l) / 2;

  if (nums[middle] == target) {
    return middle;
  }
  if (target < nums[middle]) {
    return binary_search_resursive(nums, target, l, middle - 1);
  } else {
    return binary_search_resursive(nums, target, middle + 1, r);
  }
}

int main()
{
  // Q: Find the number in the sequence and return index of number. If number not found, return -1.
  std::vector<int> nums = {0, 10, 20, 30, 40, 50};

  for (uint i = 0; i < nums.size(); ++i) {
    assert(binary_search_iterative(nums, nums[i], 0, nums.size() - 1) == i);
  }
  assert(binary_search_iterative(nums, -10, 0, nums.size() - 1) == -1);
  assert(binary_search_iterative(nums, 100, 0, nums.size() - 1) == -1);
  assert(binary_search_iterative({}, 10, 0, nums.size() - 1) == -1);

  for (uint i = 0; i < nums.size(); ++i) {
    assert(binary_search_resursive(nums, nums[i], 0, nums.size() - 1) == i);
  }
  assert(binary_search_resursive(nums, -10, 0, nums.size() - 1) == -1);
  assert(binary_search_resursive(nums, 100, 0, nums.size() - 1) == -1);
  assert(binary_search_resursive({}, 10, 0, nums.size() - 1) == -1);

  return 0;
}
