#include <iostream>

struct A {
  A() = default;
  virtual ~A() = default;
  virtual void foo() { std::cout << "A::foo()" << std::endl; }

  void bar() { std::cout << "A::bar()" << std::endl; }
};
// Final specifier
struct B final : A {
  B() = default;  // call A::A()
  // Deleted functions
  B(const B&) = delete;
  B& operator=(const B&) = delete;
  // Explicit virtual overrides
  void foo() override { std::cout << "B::foo()" << std::endl; }

  // void bar() override {} // error
};

int main() {
  A* b = new B();
  b->foo();
  b->bar();
  return 0;
}
