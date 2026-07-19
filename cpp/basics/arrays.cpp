#include <iostream>
#include <vector>
#include <algorithm>
#include <array>

int main()
{
  std::vector<int> arr{1, 2, 3};  // heap allocation
  for (int i = 0; i < arr.size(); ++i) {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;

  // arr[1000] = 10; // UB
  // arr.at(1000) = 0; // проверка границ error

  std::cout << arr.front() << " " << arr.back() << std::endl;

  arr.insert(arr.begin(), 10);  // вставка в начало O(n) 10 1 2 3
  arr.push_back(1);             // вставка в конец O(1) 10 1 2 3 1
  arr.emplace_back(2);          // вставка без лишней копииж 10 1 2 3 1 2
  arr.reserve(100);
  arr.pop_back();          // удалить один с конца O(1) 10 1 2 3 1
  arr.erase(arr.begin());  // O(n) 1 2 3 1
  arr.resize(3);           // 1 2 3

  std::cout << arr.capacity() << " " << arr.size() << std::endl;
  for (int i = 0; i < arr.size(); ++i) {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;

  std::sort(arr.begin(), arr.end());

  std::for_each(arr.begin(), arr.end(), [](int el) { std::cout << el << " "; });
  std::cout << std::endl;

  std::sort(arr.begin(), arr.end(), [](const auto& a, const auto& b) { return a > b; });

  std::for_each(arr.begin(), arr.end(), [](int el) { std::cout << el << " "; });
  std::cout << std::endl;

  int& r = arr[0];
  arr.push_back(900);  // iterator invalidation

  std::array<int, 5> stack_array{1, 2, 3, 4};  // stack allocation
  std::cout << stack_array[0] << std::endl;

  int c_arr[3] = {1, 2, 3};
  std::cout << c_arr[0] << std::endl;

  std::vector<std::vector<int>> grid(2, std::vector<int>(2, 0));
  grid[0][1] = 42;
  std::cout << grid[0][1] << std::endl;
  return 0;
}
