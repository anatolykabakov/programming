#include <iostream>

struct Foo {
  void foo() { std::cout << "Foo::foo()" << std::endl; }
};

struct Bar : Foo {
  void foo() { std::cout << "Bar::foo()" << std::endl; }
};

void bar(Foo& foo) { foo.foo(); }

int main()
{
  Foo f;
  f.foo();

  Bar b;
  b.foo();

  bar(b);
  return 0;
}
