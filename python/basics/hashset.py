#!/usr/bin/env python3

if __name__ == "__main__":
    # hashset
    s = set()
    s.add(1)
    s.add(1)
    s.add(1)
    print(s)
    s.add(2)
    print(s)
    print(1 in s)  # true
    print(3 in s)  # false
    s.remove(1)
    print(s)

    # list to set conversion
    l = [1, 1, 2, 3, 4, 4]
    s = set(l)
    print(s)

    # set comprehension
    s = {i for i in range(10)}
    print(s)
