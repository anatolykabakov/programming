# /usr/bin/env python3


def binary_search(nums, target):
    l, r = 0, len(nums) - 1

    while l <= r:
        mid = l + (r - l) // 2
        if nums[mid] == target:
            return mid

        if nums[mid] < target:
            l = mid + 1
        else:
            r = mid - 1
    return -1


if __name__ == "__main__":
    arr = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    number = 7
    print(binary_search(arr, 7))
    print(binary_search(arr, 70))
    print(binary_search(arr, -70))
