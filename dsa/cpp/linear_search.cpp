
#include <iostream>
#include <vector>

int linear_search(const std::vector<int>& array, int item)
{
  for (int i = 0; i < array.size(); ++i) {
    if (array[i] == item) {
      return i;
    }
  }
  return -1;
}

int main()
{
  std::vector<int> array = {0, 2, 4, 1, 3};

  std::cout << linear_search(array, 2) << std::endl;
  std::cout << linear_search(array, -2) << std::endl;

  return 0;
}
