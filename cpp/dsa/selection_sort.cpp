
#include <cmath>
#include <iostream>
#include <vector>

void selection_sort(std::vector<int>& array)
{
  for (int i = 0; i < array.size(); ++i) {
    for (int j = i + 1; j < array.size(); ++j) {
      if (array[j] < array[i]) {
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
      }
    }
  }
}

int main()
{
  std::vector<int> array = {0, 3, 2, 1, 4, 5};

  selection_sort(array);
  for (const auto& item : array) {
    std::cout << item << std::endl;
  }

  return 0;
}
