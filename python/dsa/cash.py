# /usr/bin/env python3

import time


def factorial(n):
    """
    3! = 1 * 2 * 3 = 6
    """
    result = 1

    for i in range(1, n + 1):
        result *= i
    return result


if __name__ == "__main__":
    result = 0
    start = time.time()
    values = [1000, 2000, 3000, 4000, 5000, 6000, 1000, 2000, 3000, 4000]
    for x in values:
        result += factorial(x)
    end = time.time() - start
    print(end)
    cash = {}
    result = 0
    start = time.time()
    for x in values:
        if not (x in cash):
            cash[x] = factorial(x)
            print("calculation")
        else:
            print("cash")
        result += cash[x]
    end = time.time() - start
    print(end)
