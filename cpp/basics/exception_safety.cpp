#include <iostream>
#include <memory>

struct Foo {
  Foo() { std::cout << "Foo ctor" << std::endl; }
  ~Foo() { std::cout << "Foo dtor" << std::endl; }
};

struct LeakCtor {
  LeakCtor()
  {
    // foo = new Foo(); // leak destructor will not called
    foo = std::make_unique<Foo>();

    throw std::runtime_error("runtime ex");
  }

  ~LeakCtor()
  {
    // if (foo) {
    //     delete foo;
    // }
  }

  // Foo *foo;
  std::unique_ptr<Foo> foo;
};

struct Bad {
  Bad() { std::cout << "Bad ctor" << std::endl; }
  ~Bad() noexcept(false)
  {
    std::cout << "~Bad ctor" << std::endl;
    throw std::runtime_error("bad ex");
  }
};

struct LeakDtor {
  LeakDtor()
  {
    // foo = new Foo(); // leak destructor will not called
    foo = std::make_unique<Bad>();
  }

  ~LeakDtor() noexcept(false)
  {
    // if (foo) {
    //     delete foo;
    // }
    // throw std::runtime_error("runtime ex");
  }

  // Foo *foo;
  std::unique_ptr<Bad> foo;
};

int main()
{
  try {
    LeakCtor leak;
  } catch (const std::runtime_error& ex) {
    std::cerr << ex.what() << std::endl;
  }

  try {
    LeakDtor leak;
  } catch (const std::runtime_error& ex) {
    std::cerr << ex.what() << std::endl;
  }
}
