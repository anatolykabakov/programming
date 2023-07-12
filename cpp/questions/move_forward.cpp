#include <iostream>
#include <utility>

int foo(int& arg) { std::cout << "foo(int&)" << std::endl; }
int foo(int&& arg) { std::cout << "foo(int&&)" << std::endl; }

template <typename T>
int a(T&& x)
{
  foo(x);
}
template <typename T>
int b(T&& x)
{
  foo(std::move(x));
}
template <typename T>
int c(T&& x)
{
  foo(std::forward<T>(x));
}

int main()
{
  int var = 1;
  a(var); // "foo(int&)"
  b(var); // foo(int&&)
  c(var); // foo(int&)
  a(1); // foo(int&)
  b(1); // foo(int&&)
  c(1); // foo(int&&)
  return 0;
}
