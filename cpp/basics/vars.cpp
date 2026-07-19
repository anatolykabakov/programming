#include <iostream>

int counter()
{
  static int count = 0;  // инициализируется только один раз
  return count++;
}

struct Foo {
  int a{0};
};

Foo& MayersSingleton()
{
  static Foo foo;
  return foo;
}

static int file_local = 42;  // internal linkage. видимость только в этом файле
int global_var = 42;  // global linkage. видимость во всем программе (extern можно использовать для доступа из других
                      // файлов или в header файле)

namespace {
int file_local_var = 42;  // то же самое что и static int file_local = 42;
}

int main()
{
  const std::string immutable = "Hello, world!";  // immutable
  // immutable = "Hello, world!2"; // error
  std::cout << immutable << std::endl;

  std::string mutable_str = "Hello, world!";
  mutable_str = "Hello, world!2";
  std::cout << mutable_str << std::endl;  // Hello, world!2

  // auto - вывод типа по значению
  auto x = 42;                                 // int
  std::cout << typeid(x).name() << std::endl;  // i int
  auto y = 3.14;                               // double
  std::cout << typeid(y).name() << std::endl;  // d - double
  auto z = std::string("Hello, world!");       // std::string
  std::cout << typeid(z).name() << std::endl;  // NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE - std::string

  auto r = "Hello, world!";                    // const char*
  std::cout << typeid(r).name() << std::endl;  // PKc - const char*

  constexpr int N = 1;                         // constexpr int
  std::cout << typeid(N).name() << std::endl;  // i int
  int arr[N];                                  // OK

  const int M = 1;
  int arr2[M];  // OK

  counter();
  counter();

  std::cout << counter() << std::endl;  // 2
  std::cout << counter() << std::endl;  // 3

  int uninit;          // undefined behavior
  int braced_init{0};  // value-initialization without
}
