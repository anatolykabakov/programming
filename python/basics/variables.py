#!/usr/bin/env python3

if __name__ == "__main__":
    # Variables are dynamicly typed
    n = 0
    print(n)

    n = "abc"
    print(n)

    # Multiple assigments
    a, b = 1, "abc"
    print(a, b)

    # Increment
    n = 0
    n = n + 1  # good
    n += 1  # good
    print(n)
    # n++ error
