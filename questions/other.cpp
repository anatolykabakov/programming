#include <iostream>

struct Foo {
  void bar() { std::cout << "bar" << std::endl; }
};

int main() {
  void* p = &p;
  std::cout << bool(p) << std::endl;

  Foo* b;
  b->bar();
  return 0;
}
