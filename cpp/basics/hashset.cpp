#include <iostream>
#include <unordered_set>
#include <vector>

void print(const std::unordered_set<int>& s)
{
  for (const auto e : s) {
    std::cout << e << " ";
  }
  std::cout << std::endl;  // not ordered 6 5 4 2 1
}

bool twoSum(const std::vector<int>& nums, int target)
{
  std::unordered_set<int> s;
  for (const auto& num : nums) {
    int diff = target - num;
    if (s.find(diff) != s.end()) {
      return true;
    }
    s.insert(num);
  }
  return false;
}

std::unordered_set<int> intersection(const std::unordered_set<int>& a, const std::unordered_set<int>& b)
{
  std::unordered_set<int> res;

  auto& small = a.size() >= b.size() ? b : a;
  auto& big = a.size() >= b.size() ? a : b;

  for (const auto& num : small) {
    if (big.find(num) != big.end()) {
      res.insert(num);
    }
  }
  return res;
}

int main()
{
  std::unordered_set<int> s;
  s.insert(42);
  s.insert(42);                           // ignore dublicate
  s.emplace(10);                          // construct in-place
  std::cout << s.count(42) << std::endl;  // 1
  s.erase(42);
  if (s.find(42) == s.end()) {
    std::cout << "0" << std::endl;
  }
  s.clear();

  std::vector<int> v{1, 1, 2, 4, 5, 6, 6};

  for (const auto& e : v) {
    auto [it, inserted] = s.insert(e);
    if (!inserted) {
      std::cout << e << " " << inserted << std::endl;
    }
  }
  print(s);

  std::unordered_set<int> s2(v.begin(), v.end());

  for (auto it = s2.begin(); it != s2.end();) {
    if (*it % 2) {
      it = s2.erase(it);
    } else {
      ++it;
    }
  }
  print(s2);

  std::cout << twoSum({6, 2, 10, 4, 5}, 11) << std::endl;

  std::unordered_set<int> nums1{1, 2, 3, 4, 5};
  std::unordered_set<int> nums2{4, 5, 6, 7};
  auto res = intersection(nums1, nums2);

  print(res);

  std::unordered_set<int> s{1, 2, 3, 4};
  s.insert(5);
  s.count(2);
  s.find(2) != s.end();
  s.erase(2);

  // dedup
  std::vector<int> v{1, 1, 2, 4, 2};
  std::unordered_set<int> s1(v.begin(), v.end());
}
