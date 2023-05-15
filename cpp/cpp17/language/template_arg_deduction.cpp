#include <iostream>

template <typename T = int>
struct Foo {
  T value;
  Foo(T init) : value{init} {}
  Foo() : value{} {}
};

int main()
{
  Foo f(1.0);  // Foo<double>
  Foo f2;      // Foo<int>
  return 0;
}
