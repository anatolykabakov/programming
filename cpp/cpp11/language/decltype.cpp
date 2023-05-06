#include <iostream>

template <typename X, typename Y>
auto sum(X x, Y y) -> decltype(x + y) {
  return x + y;
}

struct Foo {
  const int& foo() { return a; }
  int a = 0;
};

int main() {
  int x = 1;

  sum(1, 2);    // decltype(int + int) = int
  sum(1, 2.0);  // decltype(int + double) = double
  Foo f;
  auto a = f.foo();               // auto a = int
  decltype(f.foo()) b = f.foo();  // int&

  decltype(x) c = x;  // int
  const int& d = x;
  decltype(d) e = x;  // const int&

  return 0;
}
