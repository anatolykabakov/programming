#include <iostream>

struct MyObj {
  int value{123};
  auto getValueCopy()
  {
    return [*this] { return value; };
  }
  auto getValueRef()
  {
    return [this] { return value; };
  }
};

int main()
{
  // New rules for auto deduction from braced-init-list
  // auto x1 {1, 2, 3}; // error: not a single element
  auto x2 = {1, 2, 3};  // x2 is std::initializer_list<int>
  auto x3{3};           // x3 is int
  auto x4{3.0};         // x4 is double

  // constexpr lambda
  auto identity = [](int x) constexpr { return x; };
  static_assert(identity(123) == 123);

  constexpr auto add = [](int x, int y) {
    auto L = [=] { return x; };
    auto R = [=] { return y; };
    return [=] { return L() + R(); };
  };

  static_assert(add(1, 2)() == 3);

  // Lambda capture this by value
  MyObj mo;
  auto valueCopy = mo.getValueCopy();
  auto valueRef = mo.getValueRef();
  mo.value = 321;
  std::cout << valueCopy() << std::endl;  // 123
  std::cout << valueRef() << std::endl;   // 321

  return 0;
}
