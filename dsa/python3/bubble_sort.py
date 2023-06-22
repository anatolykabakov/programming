# /usr/bin/env python3


def bubble_sort(arr):
    for _ in range(0, len(arr)):
        for j in range(0, len(arr) - 1):
            if arr[j] > arr[j + 1]:
                tmp = arr[j]
                arr[j] = arr[j + 1]
                arr[j + 1] = tmp
    return arr


if __name__ == "__main__":
    print(bubble_sort([4, 2, 1, 4, 10, 6, 1, 8, 9]))
    print(bubble_sort([-4, 2, 1, 4, 10, -6, 1, 8, 9]))
    print(bubble_sort([-4]))
    print(bubble_sort([]))
