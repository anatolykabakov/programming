#include <iostream>
#include <vector>
#include <iterator>

int main()
{
  std::vector<int> v{10, 20, 30};
  for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  auto it = v.begin();
  v.push_back(4);  // invalidation if realloc

  v.erase(v.begin());  // it invalid

  std::vector<int> v2{1, 2, 2, 3, 2, 4};
  v2.erase(std::remove(v2.begin(), v2.end(), 2), v2.end());
  for (const auto& num : v2) {
    std::cout << num << std::endl;
  }
  v2.erase(std::remove_if(v2.begin(), v2.end(), [](int x) { return x % 2; }));
  for (const auto& num : v2) {
    std::cout << num << std::endl;
  }
}
