#!/usr/bin/env python3

if __name__ == "__main__":
    # hashmap aka dict
    d = dict()
    d["key1"] = 10
    d["key2"] = 20
    print(d)

    print(len(d))

    d["key1"] = 100
    print(d)

    print("key1" in d)  # true
    print("key11" in d)  # false

    d.pop("key1")
    print("key1" in d)  # false

    d = {"key10": 1000, "key2": 100110}
    print(d)

    # looping
    for key in d:
        print("key {} value {}".format(key, d[key]))

    for key, value in d.items():
        print("key {} value {}".format(key, value))

    for value in d.values():
        print("value {}".format(value))

    # comprehension
    d = {str(i): i for i in range(10)}
    print(d)
