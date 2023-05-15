#include <iostream>
#include <vector>

template <typename T>
using Vec = std::vector<T>;

void foo(int i) { std::cout << "foo(int i)" << std::endl; }

void foo(char* i) { std::cout << "foo(char* i)" << std::endl; }

enum class Alert : bool { Red, While };

enum class Color : unsigned int { Red = 0xff0000, Green = 0xff00, Blue = 0xff };

int main()
{
  // Type aliasing
  Vec<int> v;

  using string_alias = std::string;
  string_alias s = "12345";

  // nullptr
  // foo(NULL); // error
  foo(nullptr);

  // Strongly-typed enums
  Color c = Color::Red;
  // if (c == Alert::Red) {} // error

  return 0;
}
