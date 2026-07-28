#include <iostream>
#include <memory>

struct Base {
  Base()
  {
    std::cout << "Base::ctor" << std::endl;
    run();
  }

  virtual void run() { std::cout << "Base::run" << std::endl; }

  virtual ~Base()
  {
    std::cout << "~Base::dtor" << std::endl;
    run();
  }
};

struct Derived : public Base {
  Derived()
  {
    std::cout << "Derived::ctor" << std::endl;
    run();
  }

  void run() { std::cout << "Derived::run" << std::endl; }

  ~Derived()
  {
    std::cout << "~Derived::dtor" << std::endl;
    run();
  }
};

int main()
{
  std::unique_ptr<Base> b = std::make_unique<Derived>();
  b->run();
}
