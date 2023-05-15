#include <iostream>
#include <string>

class Solution {
public:
  std::string addBinary(std::string a, std::string b)
  {
    while (a.size() != b.size()) {
      if (a.size() > b.size()) {
        b = "0" + b;
      } else {
        a = "0" + a;
      }
    }
    std::string result;

    std::string carry;

    for (int i = 0; i < a.size(); ++i) {
      carry += '0';
      result += '0';
    }

    for (int i = a.size() - 1; i >= 0; --i) {
      if (a[i] == '0' && b[i] == '0') {
        result[i] = '0';
        if (carry[i] == '1') {
          result[i] = '1';
        }
      } else if (a[i] == '0' && b[i] == '1') {
        result[i] = '1';
        if (carry[i] == '1' && i > 0) {
          result[i] = '0';
          carry[i - 1] = '1';
        }
        if (carry[i] == '1' && i == 0) {
          result[i] = '0';
          result = "1" + result;
        }
      } else if (a[i] == '1' && b[i] == '0') {
        result[i] = '1';
        if (carry[i] == '1' && i > 0) {
          result[i] = '0';
          carry[i - 1] = '1';
        }
        if (carry[i] == '1' && i == 0) {
          result[i] = '0';
          result = "1" + result;
        }
      } else if (a[i] == '1' && b[i] == '1') {
        result[i] = '0';
        if (carry[i] == '1') {
          result[i] = '1';
        }
        if (i > 0) {
          carry[i - 1] = '1';
        } else {
          result = "1" + result;
        }
      }
      std::cout << "result " << result << std::endl;
    }

    return result;
  }
};

int main()
{
  Solution s;
  std::cout << s.addBinary("11", "01") << std::endl;
  std::cout << s.addBinary("1010", "1011") << std::endl;
  std::cout << s.addBinary("1", "111") << std::endl;
  std::cout << s.addBinary("11", "11") << std::endl;
  std::cout << s.addBinary("1111", "1111") << std::endl;
  return 0;
}
