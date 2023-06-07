#!/usr/bin/env python3

if __name__ == "__main__":
    # Strings
    s = "abc"
    print(s)
    print(s[0:1])

    # s[0] = "z" error. strings immutable

    s += "de"
    print(s)

    # string to number conversion
    num = int("123")
    print(num)

    num = float("123.55")
    print(num)

    # number to string conversion
    s = str(num)
    print(s)

    # obtaining a ASCII value from char
    print(ord("a"))
    print(ord("z"))

    # obtaining a char value from ASCII
    print(chr(97))

    # create the a-z alphabet
    alphabet = [chr(c) for c in range(ord("a"), ord("z") + 1)]
    print(alphabet)
