#include <exception>
#include <iostream>

[[noreturn]] void f() { throw std::logic_error("error"); }

constexpr int square(int x) { return x * x; }

int square2(int x) { return x * x; }

struct Complex {
  constexpr Complex(double r, double i) : re{r}, im{i} {}
  constexpr double real() { return re; }
  constexpr double imag() { return im; }

private:
  double re;
  double im;
};

struct Foo {
  int foo;
  Foo(int foo) : foo{foo} {}
  Foo() : Foo(0) {}
};

int main()
{
  try {
    f();
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  constexpr int a = square(2);  // calc in compile time
  // int a = square(2); // error
  int b = square2(2);  // in runtime

  const int x = 123;
  // constexpr const int& y = x; // error -- constexpr variable `y` must be initialized
  // by a constant expression

  Complex constexpr c{1.0, 2.0};
  // constexpr double r = c.real(); error

  Foo f;
  std::cout << f.foo << std::endl;
}
