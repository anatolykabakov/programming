
#include <iostream>
#include <vector>

int linear_search(const std::vector<int>& array, int item)
{
  for (int i = 0; i < array.size(); ++i) {
    if (array[i] == item) {
      return i;
    }
  }
}

int main()
{
  std::vector<int> array = {0, 2, 4, 1, 3};

  int result = linear_search(array, 2);
  std::cout << result << std::endl;

  return 0;
}
