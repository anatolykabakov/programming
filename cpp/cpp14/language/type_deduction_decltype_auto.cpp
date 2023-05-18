#include <iostream>
#include <memory>
#include <string>

// auto f(int i) {
//   return i;
// }

template <typename T>
auto& f(T& t)
{
  return t;
}

decltype(auto) g(const int& i) { return i; }

int main()
{
  // Returns a reference to a deduced type.
  auto g = [](auto& x) -> auto& { return f(x); };
  int y = 123;
  int& z = g(y);  // reference to `y`

  // decltype(auto)

  const int x = 0;
  auto x1 = x;            // int
  decltype(auto) x2 = x;  // const int
  int y = 0;
  int& y1 = y;
  auto y2 = y1;            // int
  decltype(auto) y3 = y1;  // int&
  int&& z = 0;
  auto z1 = std::move(z);            // int
  decltype(auto) z2 = std::move(z);  // int&&

  int x = 123;
  // static_assert(std::is_same<int, decltype(f(x))>::value == 0);
  static_assert(std::is_same<const int&, decltype(f(x))>::value == 1);
  static_assert(std::is_same<const int&, decltype(g(x))>::value == 1);
  return 0;
}
