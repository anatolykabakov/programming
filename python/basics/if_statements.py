#!/usr/bin/env python3

if __name__ == "__main__":

    n = 1
    if n > 0:
        n -= 1
    elif n < 0:
        n += 1
    else:
        n = 1000

    print(n)

    # Parentheses needed for multi conditions
    # and == &&
    # or == ||
    a, b = 1, 2
    if (a > 0 and b < 3) or a != b:
        print(a)
    else:
        print(b)

    """
    Priority of operations.
    Here is the complete precedence table, lowest precedence to highest.
    A row has the same precedence and groups from left to right
    0. :=
    1. lambda
    2. if – else
    3. or
    4. and
    5. not x
    6. in, not in, is, is not, <, <=, >, >=, !=, ==
    7. |
    8. ^
    9. &
    10. <<, >>
    11. +, -
    12. *, @, /, //, %
    13. +x, -x, ~x
    14. **
    14. await x
    15. x[index], x[index:index], x(arguments...), x.attribute
    16. (expressions...), [expressions...], {key: value...}, {expressions...}`
    """
