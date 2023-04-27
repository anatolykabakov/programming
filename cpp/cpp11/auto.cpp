#include <iostream>

template <typename X, typename Y>
auto add(X x, Y y) -> decltype(x + y) {
  return x + y;
}

int main() {
  // auto-typed variables are deduced by the compiler according to the type of their initializer.
  auto a = 3.14;           // double
  auto b = 1;              // int
  auto& c = b;             // int&
  auto d = {0};            // std::initializer_list<int>
  auto&& e = 1;            // int&&
  auto&& f = b;            // int&
  auto g = new auto(123);  // int*
  const auto h = 1;        // const int
  auto i = 1, j = 2;       // int int
  // auto l=1, m=2.0; //error

  std::cout << add(1, 2) << std::endl;
  std::cout << add(1.0, 2.0) << std::endl;

  return 0;
}
