from math import floor


def quick_sort(arr):
    if len(arr) <= 1:
        return arr
    middle_index = floor(len(arr) / 2)
    left = []
    right = []
    middle = []
    for i in range(0, len(arr)):
        if arr[i] < arr[middle_index]:
            left.append(arr[i])
        if arr[i] > arr[middle_index]:
            right.append(arr[i])
        if arr[i] == arr[middle_index]:
            middle.append(arr[i])
    return quick_sort(left) + middle + quick_sort(right)


if __name__ == "__main__":
    arr = [4, 2, 1, 4, 10, 6, 1, 8, 9]
    sorted_arr = quick_sort(arr)
    print(sorted_arr)
