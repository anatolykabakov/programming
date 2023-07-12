# /usr/bin/env python3


def linear_search(arr, number):
    for i in range(0, len(arr)):
        if arr[i] == number:
            return i
    return None


if __name__ == "__main__":
    print(linear_search([1, 2, 3, 4, 5, 6, 7, 8, 9], 7))
    print(linear_search([1, 2, 3, 4, 5, 6, 7, 8, 9], -7))
    print(linear_search([1, 2, 3, 4, 5, 6, 7, 8, 9], 78))
