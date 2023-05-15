#include <iostream>
#include <unordered_map>

[[nodiscard]] int foo() { return 1; }

int main()
{
  foo();  // warning: ignoring return value of 'int foo()', declared with attribute
          // nodiscard
  return 0;
}
