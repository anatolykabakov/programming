#include <iostream>

void passByValue(int arg) { arg = 42; }

void passByPointer(int* arg)
{
  if (arg != nullptr) {
    *arg = 42;
  }
}

void passByRef(int& arg) { arg = 42; }

void passByConstRef(const int& arg)
{
  // arg = 42; error
  std::cout << arg << std::endl;
}

void swapByRef(int& a, int& b)
{
  int tmp = a;
  a = b;
  b = tmp;
}

void swapByPointer(int* a, int* b)
{
  if (a == nullptr || b == nullptr) {
    return;
  }
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

int main()
{
  int x = 42;
  int* p = &x;
  std::cout << *p << std::endl;

  *p = 100;
  std::cout << *p << std::endl;

  int& ref = x;
  ref = 10;
  std::cout << ref << std::endl;

  int a = 10;
  int b = 20;

  const int* p1 = &a;
  // *p1 = 5; error
  p1 = &b;

  std::cout << *p1 << std::endl;

  int* const p2 = &a;
  *p2 = 1;
  // p2 = &b; //  error: assignment of read-only variable ‘p2’
  std::cout << *p2 << std::endl;

  const int* const p3 = &x;
  // *p3 = 2 error
  // p3 = &b error
  std::cout << x << std::endl;

  passByValue(x);

  std::cout << x << std::endl;  // 10

  passByRef(x);

  std::cout << x << std::endl;  // 42

  x = 10;

  passByConstRef(x);  // 10

  passByPointer(&x);

  std::cout << x << std::endl;

  int arr[3] = {10, 20, 30};

  int* p4 = arr;
  for (int i = 0; i < 3; ++i) {
    std::cout << p4[i] << " ";
  }
  std::cout << std::endl;

  std::cout << a << " " << b << std::endl;

  swapByRef(a, b);

  std::cout << a << " " << b << std::endl;

  swapByPointer(&a, &b);

  std::cout << a << " " << b << std::endl;
  return 0;
}
