#include <iostream>

class Base {
public:
  ~Base() { std::cout << "Hello from ~Base()" << std::endl; }
  // virtual ~Base() { std::cout << "Hello from ~Base()" << std::endl; }
};

class Derived : public Base {
public:
  virtual ~Derived()
  {
    // Здесь могла бы быть очистка ресурсов
    std::cout << "Hello from ~Derived()" << std::endl;
  }
};

int main()
{
  Base* b = new Derived();
  delete b;
  return 0;
}
