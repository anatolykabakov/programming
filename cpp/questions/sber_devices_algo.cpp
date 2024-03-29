// Реализовать функцию, которая удаляет четные числа из std::vector(inplace)
// Порядок нечетных элементов остается неизменным
// Пример: [2, 3, 1, 6, 7, 8] -> [3, 1, 7]
// Time O(N) Space O(1)

// f([8, 3, 1, 6, 7, 2]) = [3, 1, 6, 7, 8, 2]
// f([2, 3, 1]) = [3, 1]
// f([2, 3, 1, 2]) = [3, 1]
// f([2, 2]) = []
// f([1, 1]) = [1, 1]
// f([1]) = [1]
// f([]) = []
// [2, 4, 6, 1, 3, 8] = [1]
// l = 0

// 1. Проход по массиву
// 2. Посчитать количество четных элементов
// 3. Переместить четные элементы в конец массива.
// 4. Вычислить новый размер массива = текущий - количество четных элементов
// 5. Используем функцию resize для обрезки массива

// Time O(n) Space O(1)
void remove_even(std::vector<int>& nums)
{
  std::size_t odd_count = 0 for (auto i = 0; i < nums.size(); ++i)
  {
    if (input[i] % 2) {
      std::swap(nums[i], nums[odd_count]);
      ++odd_count;
    }
  }
  nums.resize(odd_count);
}
