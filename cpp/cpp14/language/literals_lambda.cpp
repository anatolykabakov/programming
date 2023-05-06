#include <string>
#include <iostream>
#include <memory>

int factory(int i) { return i * 10; };

int main() {
  // Binary literals
  0b110;        // == 6
  0b1111'1111;  // == 255

  // Generic lambda expressions
  auto identity = [](auto x) {
    std::cout << x << std::endl;
    return x;
  };
  int three = identity(3);            // == 3
  std::string foo = identity("foo");  // == "foo"

  // Lambda capture initializers
  auto f = [x = factory(2)]() { return x; };
  std::cout << f() << std::endl;

  auto generator = [x = 0]() mutable { return ++x; };
  std::cout << generator() << std::endl;
  std::cout << generator() << std::endl;
  std::cout << generator() << std::endl;

  auto p = std::make_unique<int>(1);

  // auto task1 = [=] { *p = 5; }; // ERROR: std::unique_ptr cannot be copied
  // vs.
  auto task2 = [p = std::move(p)] { *p = 5; };  // OK: p is move-constructed into the closure object
  // the original p is empty after task2 is created

  // Using this reference-captures can have different names than the referenced variable.
  auto x = 1;
  auto g = [& r = x, x = x * 10] {
    ++r;
    return r + x;
  };
  g();  // sets x to 2 and returns 12

  return 0;
}
