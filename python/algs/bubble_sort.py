"""
Сортировка пузырьков -- алгоритм сортировки последовательности чисел.
Описание:
1. Проходим в цикле по массиву n раз где n количество элментов, индекс i до конца массива
2. Если следующий элемент итерации меньше чем текущий, то меняем из местами
Сложность алгоритма O(n*n)
"""


def bubble_sort(arr):
    count = 0  # счетчик для оценки сложности
    for _ in range(0, len(arr)):
        for j in range(0, len(arr) - 1):
            if arr[j] > arr[j + 1]:
                tmp = arr[j]
                arr[j] = arr[j + 1]
                arr[j + 1] = tmp
            count += 1
    return arr, count


if __name__ == "__main__":
    arr = [4, 2, 1, 4, 10, 6, 1, 8, 9]
    number = 7
    sorted_arr, count = bubble_sort(arr)
    print(sorted_arr, count)