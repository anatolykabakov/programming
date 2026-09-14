#include <iostream>
#include <vector>
#include <string>

void printAll(const std::vector<std::string>& arr)
{
  for (const auto& v : arr) {
    std::cout << v << " ";
  }
  std::cout << std::endl;
}

int main()
{
  const std::vector<std::string> immutable_list = {"apple", "banana", "orange"};
  // immutable_list.push_back("red"); // error: passing ‘const
  std::vector<std::string> mutable_list = immutable_list;  // copy
  mutable_list.push_back("red");

  for (const auto& value : mutable_list) {
    std::cout << value << std::endl;
  }  // apple banana orange red

  size_t i{0};
  while (i < mutable_list.size()) {
    std::cout << mutable_list[i] << std::endl;
    ++i;
  }

  int n = 1;
  while (n < 5) {
    std::cout << n << " ";
    ++n;
  }

  std::cout << std::endl;

  for (int i = 0; i < 5; ++i) {
    std::cout << i << " ";
  }

  std::cout << std::endl;

  for (int i = 5; i >= 0; --i) {
    if (i == 3) {
      continue;
    }
    std::cout << i << " ";
  }
  std::cout << std::endl;

  int x = 0;
  do {
    std::cout << x << " ";
    ++x;
  } while (x <= 3);

  std::cout << std::endl;

  for (auto& v : mutable_list) {
    v += "1";

    std::cout << v << std::endl;
  }

  printAll(immutable_list);

  for (int i = 0; i < 5; ++i) {
    if (i == 2) {
      break;
    }
    std::cout << i << std::endl;
  }
}
