#include <iostream>
#include <unordered_map>
#include <functional>

int simple_cached_func(std::function<int(int)> f, int arg) {
  static std::unordered_map<int, int> cache;
  if (cache.find(arg) != cache.end()) {
    std::cout << "cached value " << cache[arg] << std::endl;
    return cache[arg];
  }
  auto result = f(arg);
  cache[arg] = result;
  std::cout << "value " << result << std::endl;
  return result;
}

int fibonachi(int n) {
  if (n < 2)
    return n;
  return simple_cached_func(fibonachi, n - 2) + simple_cached_func(fibonachi, n - 1);
}

int main() {
  std::cout << fibonachi(5) << std::endl;
  return 0;
}
