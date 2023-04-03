/**
 * Given an integer x, return true if x is a palindrome, and false otherwise
*/

#include <vector>
#include <iostream>

class Solution {
public:
    bool isPalindrome(int x) {
      if (x < 0) { 
        return false;
      }
      const auto num_str = std::to_string(x);

      auto half_size = num_str.size() / 2;
      
      for (int i=0; i < half_size; ++i) {
        auto start = num_str[i];
        auto end = num_str[num_str.size() - i - 1];
        if (start != end) {
          return false;
        }
        
      }
      return true;
    }
};

int main() {
  Solution solution;
  bool result = solution.isPalindrome(17712);
  std::cout << result << std::endl;
  
  return 0;
}