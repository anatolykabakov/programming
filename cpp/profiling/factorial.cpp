#include <cstdlib>
#include <iostream>
// 3! = 3 * 2 * 1 = 6
// 4! = 4 * 3 * 2 * 1 = 24
unsigned long long factorial(unsigned long long n)
{
  if (n == 1) {
    return n;
  }
  return n * factorial(n - 1);
}

void foo(unsigned long long n)
{
  for (unsigned long long i = 1; i < n; ++i) {
    factorial(i);
  }
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    return -1;
  }
  foo(std::atoi(argv[1]));
  return 0;
}
