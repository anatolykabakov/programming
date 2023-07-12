#include <functional>
#include <iostream>
#include <unordered_map>

int memoization(std::function<int(int)> f, int arg)
{
  static std::unordered_map<int, int> cache;
  if (cache.find(arg) != cache.end()) {
    std::cout << "value from cache " << cache[arg] << std::endl;
    return cache[arg];
  }
  auto result = f(arg);
  cache[arg] = result;
  std::cout << "calculated value " << result << std::endl;
  return result;
}

int fibonachi(int n)
{
  if (n < 2)
    return n;
  return memoization(fibonachi, n - 2) + memoization(fibonachi, n - 1);
}

int main()
{
  std::cout << fibonachi(5) << std::endl;
  return 0;
}
