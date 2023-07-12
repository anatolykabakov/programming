
#include <iostream>
#include <vector>
#include <cassert>

/**
 * @brief This function looks up the target number in the sequence of integers.
 * If the target number found, the index of target is returned, otherwise -1.
 *
 * @param nums std::vector<int> The sequence of integers. Elements must be distinct.
 * @param target int The number to be found.
 * @return int The nums's index, if target to be found, otherwise -1.
 * @details Time O(n) Space O(1)
 */
int linear_search(const std::vector<int>& nums, int target)
{
  for (int i = 0; i < nums.size(); ++i) {
    if (nums[i] == target) {
      return i;
    }
  }
  return -1;
}

int main()
{
  // Q: Find the number in the sequence and return index of number. If number not found, return -1.
  assert(linear_search({0, 2, 4, 1, 3}, 2) == 1);
  assert(linear_search({0, -2, -4, -1, 3}, -1) == 3);
  assert(linear_search({0, 2, 4, 1, 3}, -2) == -1);
  assert(linear_search({}, -2) == -1);

  return 0;
}
