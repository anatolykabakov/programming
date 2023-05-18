#include <iostream>

class A {
public:
  A() { printFromConstructor(); }

  virtual void printFromConstructor() { std::cout << "A"; }

  void work() { print(); };

  virtual void print() { std::cout << "[A]"; };

  virtual void printFromDestructor() { std::cout << "~A"; }

  virtual ~A() { printFromDestructor(); };

private:
  // Foo f;
};

class B : public A {
public:
  B() { printFromConstructor(); };

  virtual void printFromConstructor() { std::cout << "B"; }

  virtual void printFromDestructor() { std::cout << "~B"; }

  virtual void print() { std::cout << "[B]"; }

  ~B() { printFromDestructor(); };
};

// class A {
//   rele
//   reset(Foo *) {}
//   shared_ptr<Foo> p1
// };

int main()
{
  A* pa = new B();
  pa->work();
  delete pa;
  //  foo(shared_ptr(new Foo), foo_throw())
  // try {
  //   // pa = std::make_unique<B>();
  //   pa->work();
  // } catch (const std::exception &ex) {
  //   std::cout << ex.what() << std::endl;
  // } catch (...) {
  //   std::cout << "un" << std::endl;

  // }
  // shared_ptr<Foo> p;

  return 0;
}

// A B [B] ~B ~A
