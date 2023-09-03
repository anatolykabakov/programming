#include <iostream>

class Base {
public:
  virtual void function1() { std::cout << "Base::function1" << std::endl; };
  virtual void function2() { std::cout << "Base::function2" << std::endl; };
};

class Foo : public Base {
public:
  void function1() override { std::cout << "Foo::function1" << std::endl; };
};

class Bar : public Base {
public:
  void function2() override { std::cout << "Bar::function2" << std::endl; };
};

int main()
{
  Bar* b = new Bar();
  b->function1();

  Foo* f = new Foo();
  f->function1();

  Base* bf = new Foo();
  bf->function1();

  Foo f1{};
  Base* f2 = &f1;
  f2->function1();
  return 0;
}
