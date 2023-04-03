#include <iostream>
#include <map>

//13. Roman to Integer

/**
 * Input: s = "III"
  Output: 3
  Explanation: III = 3.
 * 
 */
class Solution {
public:
    Solution() {
      mapping_.insert(std::make_pair("I", 1));
      mapping_.insert(std::make_pair("V", 5));
      mapping_.insert(std::make_pair("X", 10));
      mapping_.insert(std::make_pair("L", 50));
      mapping_.insert(std::make_pair("C", 100));
      mapping_.insert(std::make_pair("D", 500));
      mapping_.insert(std::make_pair("M", 1000));
    }

    int romanToInt(std::string s) {
      if (s.size() == 1) {
        return mapping_.at(s);
      }

      int sum = 0;
      int i = 1;
      while (i<s.size())
      {
        std::string roman{s[i]};      
        std::string prev{s[i - 1]};
        if (prev == "I" && roman == "V") {
          sum += 4;
          i+=2;
        } else if (prev == "I" && roman == "X") {
          sum += 9;
          i+=2;
        } else if (prev == "X" && roman == "L") {
          sum += 40;
          i+=2;
        } else if (prev == "X" && roman == "C") {
          sum += 90;
          i+=2;
        } else if (prev == "C" && roman == "D") {
          sum += 400;
          i+=2;
        } else if (prev == "C" && roman == "M") {
          sum += 900;
          i+=2;
        } else {
          sum += mapping_.at(roman);
          i+=1;
        }
      }
      
     
      return sum;
    }

private:
  std::map<std::string, int> mapping_;
};

int main() {
  Solution solution;
  std::cout << solution.romanToInt("III") << std::endl;
  return 0;
}