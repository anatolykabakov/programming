#include <iostream>
#include <numeric>

template <typename... T>
struct arity {
  constexpr static int value = sizeof...(T);
};

template <typename F, typename... Args>
auto sum(const F first, const Args... args) -> decltype(first) {
  const auto values = {first, args...};
  return std::accumulate(values.begin(), values.end(), F{0});
}

int main() {
  // Static assertions
  static_assert(arity<int>::value == 1);
  static_assert(arity<int, double, char>::value == 3);
  // Variadic templates
  std::cout << sum(1.0, 2.0, 3.0) << std::endl;
  return 0;
}
