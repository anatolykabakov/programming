#include <iostream>
#include <numeric>

int sum(const std::initializer_list<int>& list)
{
  int total = 0;
  for (auto& e : list) {
    total += e;
  }

  return total;
}

int main()
{
  std::cout << sum({1, 2, 3}) << std::endl;
  std::cout << sum({}) << std::endl;
  return 0;
}
