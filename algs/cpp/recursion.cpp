
#include <iostream>

int factorial(int i) {
  if (i == 1) {
    return 1;
  }

  return i * factorial(i - 1);
}
// 1, 1, 2, 3, 5, 8
int fibonachi(int i) {
  if (i == 1 || i == 2) {
    return 1;
  }
  return fibonachi(i - 1) + fibonachi(i - 2);
}

int main() {
  std::cout << "factorial is " << factorial(3) << std::endl;
  std::cout << "fibonachi is " << fibonachi(4) << std::endl;
  return 0;
}