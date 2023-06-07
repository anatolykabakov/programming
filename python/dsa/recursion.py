# /usr/bin/env python3


def factorial(n):
    """
    2! = 1 * 2 = 2
    """

    if n == 1:
        return 1

    return n * factorial(n - 1)


def fibonachi(n):
    """
    1, 1, 2, 3, 5, 8, 13
    """

    if (n == 1) or (n == 2):
        return 1

    return fibonachi(n - 1) + fibonachi(n - 2)


if __name__ == "__main__":
    number = 6
    n = factorial(number)
    print(n)
    n = fibonachi(number)
    print(n)
