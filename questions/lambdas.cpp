#include <iostream>
void foo(int a = []() {
  static int b = 1;
  return b++;
}()) {
  std::cout << a << std::endl;
}

int main() {
  foo();
  foo();

  for (int i = 0; i < 10; ++i) {
    std::cout << i << " ";
  }
  std::cout << std::endl;

  for (int i = 0; i < 10; i++) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
  return 0;
}
