# /usr/bin/env python3


def selection_sort(arr):
    count = 0  # счетчик для оценки сложности
    for i in range(0, len(arr)):
        min_element_index = i
        for j in range(i, len(arr)):
            if arr[j] < arr[min_element_index]:
                min_element_index = j
        tmp = arr[i]
        arr[i] = arr[min_element_index]
        arr[min_element_index] = tmp
        count += 1
    return arr, count


if __name__ == "__main__":
    arr = [4, 2, 1, 4, 10, 6, 1, 8, 9]
    number = 7
    sorted_arr, count = selection_sort(arr)
    print(sorted_arr)
    print(count)
