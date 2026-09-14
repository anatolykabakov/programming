#include <iostream>
#include <memory>

struct Foo {
  Foo() { std::cout << "Foo ctor" << std::endl; }
  virtual ~Foo() { std::cout << "Foo dtor" << std::endl; }
};

struct Bar : public Foo {
  Bar() { std::cout << "Bar ctor" << std::endl; }
  ~Bar() { std::cout << "Bar dtor" << std::endl; }
};

int main() { std::unique_ptr<Foo> foo = std::make_unique<Bar>(); }
