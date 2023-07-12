#include <iostream>

class Base {
public:
  ~Base() { std::cout << "Hello from ~Base()" << std::endl; }
  // virtual ~Base() { std::cout << "Hello from ~Base()" << std::endl; }
};

class Derived : public Base {
public:
  virtual ~Derived() { std::cout << "Hello from ~Derived()" << std::endl; }
};

int main()
{
  Base* b = new Derived();
  delete b;  // Hello from ~Base()
  return 0;
}  // leak
