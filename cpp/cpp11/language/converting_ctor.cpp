#include <iostream>

struct A {
  A(int) { std::cout << "A(int)" << std::endl; }
  A(int, int) { std::cout << "A(int, int)" << std::endl; }
  A(int, int, int) { std::cout << "A(int, int, int)" << std::endl; }
};

int main()
{
  // A e{1.1}; // error
  A a{0, 0};     // calls A::A(int, int)
  A b(0, 0);     // calls A::A(int, int)
  A c = {0, 0};  // calls A::A(int, int)
  A d{0, 0, 0};  // calls A::A(int, int, int)
}
