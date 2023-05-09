/**
 * Given an array of integers nums and an integer target,
 * return indices of the two numbers such that they add up to target.
 * You may assume that each input would have exactly one solution,
 * and you may not use the same element twice. You can return the answer in any order.
 * Input: nums = [2,7,11,15], target = 9
 * Output: [0,1]
 * Explanation: Because nums[0] + nums[1] == 9, we return [0, 1].
 */

#include <algorithm>
#include <iostream>
#include <stack>
#include <vector>
// class Solution {
// public:
//   std::vector<int> twoSum(std::vector<int>& nums, int target) {
//     std::vector<int> result;
//     for (int i = 0; i < nums.size(); ++i) {
//       for (int j = i + 1; j < nums.size(); ++j) {
//         if ((nums[i] + nums[j]) == target) {
//           result.push_back(i);
//           result.push_back(j);
//           break;
//         }
//       }
//     }
//     return result;
//   }
// };
using namespace std;
class Solution {
public:
    string decodeString(string s) {
        stack<char> st;

        for (int i = 0; i < s.size(); ++i) {
            if (s[i] == ']') {
                string str;
                while (st.top() != '[') {
                    char c = st.top();
                    st.pop();
                    str += c;
                }
                reverse(str.begin(), str.end());

                st.pop();  // pop [

                string digit_str;
                while (!st.empty() && st.top() <= '9' && st.top() >= '0') {
                    char c = st.top();
                    st.pop();
                    digit_str += c;
                }

                reverse(digit_str.begin(), digit_str.end());

                int k = stoi(digit_str);

                string res;
                for (int j = 0; j < k; ++j) {
                    res += str;
                }
                for (int j = 0; j < res.size(); ++j) {
                    st.push(res[j]);
                }

            } else {
                st.push(s[i]);
            }
        }

        string ans;
        while (!st.empty()) {
            char c = st.top();
            ans += c;
            st.pop();
        }

        reverse(ans.begin(), ans.end());

        return ans;
    }
};

int main() {
    std::vector<int> array = {2, 7, 11, 15};
    Solution solution;
    cout << solution.decodeString("100[leetcode]") << endl;

    // for (const auto& item : indices) {
    //   std::cout << item << std::endl;
    // }

    return 0;
}
