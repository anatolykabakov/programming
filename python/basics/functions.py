#!/usr/bin/env python3


def foo():
    print("foo")


def bar(nums, value):
    print("bar")

    def baz():
        print("baz")
        # modifying array works
        nums[0] = 100

        # modifying variables not works without nonlocal keyword
        nonlocal value
        value = 100

    baz()
    print(nums, value)


if __name__ == "__main__":
    foo()
    nums = [1, 2, 3]
    val = 10
    bar(nums, val)
    print(nums, val)
