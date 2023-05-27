#include <iostream>

// 1 1 2 3 5
// Time O(2^N) Space O(1)
int fibonachi_recursion(int n)
{
  if (n == 1 || n == 2) {
    return 1;
  }
  return fibonachi_recursion(n - 2) + fibonachi_recursion(n - 1);
}

// Time O(N) Space O(1)
int fibonachi_dynamic_programming(int n)
{
  if (n == 1 || n == 2) {
    return 1;
  }

  int one = 0;
  int two = 1;
  int i = 2;
  while (i <= n) {
    int tmp = two;
    two = one + two;
    one = tmp;
    i++;
  }
  return two;
}

int main()
{
  std::cout << fibonachi_recursion(5) << std::endl;
  std::cout << fibonachi_dynamic_programming(5) << std::endl;
}
