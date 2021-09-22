

"""
Сортировка Хоара или Быстрая -- алгоритм сортировки последовательности чисел, наиболее эффективный.
Описание: 
1. Находим серединный элемент
2. Разделяем последовательность на
   Элементы, которые меньше серединного элемента. Эти элементы всегла слева.
   Элементы, которые больше серединного элемента. Справа
   Серединный элемент. Посередине.
3. Пока длина входного массива не больше 1, Соединяем левый, серединный и правый последовательности и возвращаем

Сложность O(log2n*n)
"""

from math import floor

def quick_sort(arr):
    if (len(arr) <= 1):
        return arr
    middle_index = floor(len(arr)/2)
    left = []
    right = []
    middle = []
    for i in range(0, len(arr)):
        if (arr[i] < arr[middle_index]):
            left.append(arr[i])
        if (arr[i] > arr[middle_index]):
            right.append(arr[i])
        if (arr[i] == arr[middle_index]):
            middle.append(arr[i])
    return quick_sort(left) + middle + quick_sort(right)

if __name__ == '__main__':
    arr = [4,2,1,4,10,6,1,8,9]
    sorted_arr = quick_sort(arr)
    print(sorted_arr)