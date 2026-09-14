

#include <algorithm>
#include <vector>
#include <iostream>
#include <numeric>

void print(const std::vector<int>& nums)
{
  for (const auto& num : nums) {
    std::cout << num << " ";
  }
  std::cout << std::endl;
}
// 0,1,2,3,4
// 1,2,3,4,5
int binary_search(const std::vector<int>& nums, int target)
{
  if (nums.empty()) {
    return -1;
  }
  int l{0};
  int r{nums.size() - 1};
  while (l < r) {
    int mid = (r + l) / 2;  //
    if (nums[mid] == target) {
      return mid;
    } else if (nums[mid] < target) {
      l = mid;
    } else if (nums[mid] > target) {
      r = mid;
    }
  }
  return -1;
}

int main()
{
  // find / count
  std::vector<int> v{2, 2, 3, 1, 4, 6};
  auto it = std::find(v.begin(), v.end(), 4);
  if (it != v.end()) {
    std::cout << *it << std::endl;  // 4
  }

  auto it2 = std::find_if(v.begin(), v.end(), [](int x) { return x > 3; });
  if (it2 != v.end()) {
    std::cout << *it2 << std::endl;  // 4
  }

  // sort + custom cmp
  std::sort(v.begin(), v.end());  // ascending 1 2 3 4 6
  print(v);
  std::sort(v.begin(), v.end(), std::greater<int>());  // 6 4 3 2 1
  print(v);
  std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
  print(v);

  // binary search
  std::sort(v.begin(), v.end());
  bool found = std::binary_search(v.begin(), v.end(), 4);
  std::cout << found << std::endl;  // 1

  std::cout << binary_search(v, 4) << std::endl;  // 3
  std::cout << binary_search(v, 2) << std::endl;  // 1
  std::cout << binary_search(v, 1) << std::endl;  // 0
  std::cout << binary_search(v, 0) << std::endl;  // -1

  // transform
  std::vector<int> out(v.size());
  std::transform(v.begin(), v.end(), out.begin(), [](int x) { return x * 2; });
  print(out);  // 2 4 6 8 12

  std::vector<int> evens;
  std::copy_if(v.begin(), v.end(), std::back_inserter(evens), [](int x) { return x % 2 == 0; });
  print(evens);

  auto res = std::accumulate(v.begin(), v.end(), 0);
  std::cout << res << std::endl;
  auto [min, max] = std::minmax_element(v.begin(), v.end());
  std::cout << *min << " " << *max << std::endl;  // 1 6
  v.erase(std::unique(v.begin(), v.end()), v.end());
  print(v);
}
