#!/usr/bin/env python3

if __name__ == "__main__":
    # Arrays
    array = [1, 2, 3]
    for i in range(len(array)):
        print(array[i])

    array[0] = 0
    array.insert(0, 10)

    # reverse
    array.reverse()
    print(array)

    # Can be used as a stack
    array = []
    array.append(1)
    array.append(2)
    print(array)
    array.pop()
    print(array)

    # Initialize arrays
    array = [1] * 5
    print(array)

    array = [i for i in range(10)]
    print(array)

    # -1 it is the last value
    print(array[-1])

    # -2 second to the last value
    print(array[-2])

    # slicing. last index is non inclusive
    print(array[1:5])  # [1, 2, 3, 4]

    # unpacking

    a, b, c = [1, 2, 3]
    print(a, b, c)

    # loop with index and value
    for index, value in enumerate(array):
        print("index {0} value {1}".format(index, value))

    # Loop through multiple arrays
    arr1 = [1, 2, 3]
    arr2 = [4, 5, 6]
    for n1, n2 in zip(arr1, arr2):
        print(n1, n2)

    # sorting
    array.reverse()
    print(array)
    array.sort()
    print(array)

    array.sort(reverse=True)
    print(array)

    # custom sort
    arr = ["foo", "bo", "fooo", "f"]
    arr.sort(key=lambda x: len(x))
    print(arr)

    # 2-d list
    arr2d = [[0] * 4 for i in range(4)]
    print(arr2d)
