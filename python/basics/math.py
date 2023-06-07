#!/usr/bin/env python3

if __name__ == "__main__":
    # Default division is decimal.
    a = 5 / 2
    print(a)  # 2.5
    # Double slash rounds down.
    a = 5 // 2
    print(a)  # 2
    # Double slash rounds down with negative numbers/
    a = -5 // 2
    print(a)  # -3
    # Use decimal division to round up negative numbers, then convert to an integer.
    a = int(-5 / 2)
    print(a)  # -2
    # Modding for positive numbers
    a = 5 % 2
    print(a)  # 1
    # Modding for negative numbers
    a = -5 % 2
    print(a)  # 1
    # Except for negative numbers
    a = -10 % 3
    print(a)  # 2
    # math module
    import math

    print(math.fmod(10, 3))  # 1.0
    print(math.fmod(-10, 3))  # -1.0
    print(math.floor(3 / 2))  # 1
    print(math.ceil(3 / 2))  # 2
    print(math.sqrt(4))  # 2.0
    print(math.pow(2, 2))  # 4.0
    # Python numbers are infinite so they never overflow
    print(math.pow(2, 200))
    # But still less than infinity
    print(math.pow(2, 200) < float("inf"))
    # numpy module
    import numpy as np

    A = np.array([[6, 7], [8, 9]])
    B = np.array([[1, 3], [5, 7]])
    # Matrix multiplication. The order matter.
    C = np.dot(A, B)
    print(C)  # [[41 67] [53 87]]
    C = A @ B
    print(C)  # [[41 67] [53 87]]
    C = np.dot(B, A)
    print(C)  # [[30 34] [86 98]]
    # Element wise multiplication
    print(np.multiply(A, B))  # [[ 6 21] [40 63]]
    # inverse matrix
    print(np.linalg.inv(B))
    # determinant
    print(np.linalg.det(B))
    # matrix division
    C = np.dot(A, np.linalg.inv(B))
    print(C)  # [[-0.875  1.375] [-1.375  1.875]]
