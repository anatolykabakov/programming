#include <iostream>
#include <utility>
#include <vector>
#include <chrono>

int foo(int& arg) { std::cout << "foo(int&)" << std::endl; }
int foo(int&& arg) { std::cout << "foo(int&&)" << std::endl; }

template <typename T>
int a(T&& x)
{
  foo(x);
}
template <typename T>
int b(T&& x)
{
  foo(std::move(x));
}
template <typename T>
int c(T&& x)
{
  foo(std::forward<T>(x));
}

void move_foo(std::string&& text) { std::cout << "foo(std::string&& text)" << std::endl; }

void copy_foo(std::string text) { std::cout << "foo(std::string text)" << std::endl; }

int main()
{
  int var = 1;
  a(var);  // "foo(int&)"
  b(var);  // foo(int&&)
  c(var);  // foo(int&)
  a(1);    // foo(int&)
  b(1);    // foo(int&&)
  c(1);    // foo(int&&)

  std::string text;
  text = "1234";
  // move_foo(text); error
  move_foo(std::move(text));
  copy_foo(text);

  std::vector<int> v(1e8, 0);
  auto t1 = std::chrono::high_resolution_clock::now();
  std::vector<int> v2(v);  // copy operation
  auto t2 = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms for copy 10^8 ints"
            << std::endl;

  t1 = std::chrono::high_resolution_clock::now();
  std::vector<int> v3(std::move(v));  // copy operation
  t2 = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms for move 10^8 ints"
            << std::endl;
  return 0;
}
