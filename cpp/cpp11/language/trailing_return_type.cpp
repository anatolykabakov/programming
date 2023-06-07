#include <iostream>

// int f() {
//   return 123;
// }
// vs.
auto f() -> int { return 123; }

// NOTE: This does not compile!
// template <typename T, typename U>
// decltype(a + b) add(T a, U b) {
//     return a + b;
// }

// Trailing return types allows this:
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b)
{
  return a + b;
}

int main()
{
  std::cout << f() << std::endl;
  std::cout << add(1, 2) << std::endl;
  return 0;
}
