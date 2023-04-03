/**
 * Given an array of integers nums and an integer target, 
 * return indices of the two numbers such that they add up to target.
 * You may assume that each input would have exactly one solution, 
 * and you may not use the same element twice. You can return the answer in any order.
 * Input: nums = [2,7,11,15], target = 9
 * Output: [0,1]
 * Explanation: Because nums[0] + nums[1] == 9, we return [0, 1].
*/

#include <vector>
#include <iostream>

class Solution {
public:
    std::vector<int> twoSum(std::vector<int>& nums, int target) {
      std::vector<int> result;
      for (int i = 0; i < nums.size(); ++i) {
        for (int j = i + 1; j < nums.size(); ++j) {

          if ((nums[i] + nums[j]) == target) {
            result.push_back(i); 
            result.push_back(j); 
            break;
          }
        }
      }
      return result;
        
    }
};

int main() {
  std::vector<int> array = {2, 7, 11, 15};
  Solution solution;
  auto indices = solution.twoSum(array, 17);

  for (const auto& item: indices) {
    std::cout << item << std::endl;
  }
  
  return 0;
}