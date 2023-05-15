#include <iostream>

template <typename T>
void foo(T arg)
{
  static int i = 0;
  i++;
  std::cout << i << std::endl;
}

template <typename T>
T sum(T arg)
{
  return arg;
}

template <typename T, typename... Args>
T sum(T arg, Args... args)
{
  return arg + sum<T>(args...);
}

int main()
{
  foo(1);
  foo(1);
  foo(1.0);

  auto n1 = sum(0.5, 1, 0.5, 1);  // double 0.5 + (1 + (0.5 + 1))
  auto n2 = sum(1, 0.5, 1, 0.5);  // int 1 + (0 + (0 + 1))

  std::cout << n1 << " " << n2 << std::endl;
  return 0;
}
