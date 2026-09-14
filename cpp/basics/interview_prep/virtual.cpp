#include <iostream>
#include <memory>

struct Base {
  Base()
  {
    std::cout << "Base::ctor" << std::endl;
    log();
  }
  virtual void log() { std::cout << "Base::log" << std::endl; }
  virtual ~Base()
  {
    std::cout << "Base::dtor" << std::endl;
    log();
  }
};

struct Derived : public Base {
  Derived()
  {
    std::cout << "Derived::ctor" << std::endl;
    log();
  }
  void log() override { std::cout << "Derived::log" << std::endl; }
  ~Derived()
  {
    std::cout << "Derived::dtor" << std::endl;
    log();
  }
};

int main()
{
  std::unique_ptr<Base> a = std::make_unique<Derived>();
  a->log();  // Derived::log

  Base b = Derived();
  b.log();  // Base::log
}
