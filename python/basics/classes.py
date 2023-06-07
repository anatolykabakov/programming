#!/usr/bin/env python3


class Foo:
    # Constructor
    def __init__(self, nums):
        # public variables
        self.nums = nums
        # private variables
        self.__size = len(nums)

    def size(self):
        return self.__size

    def double_size(self):
        return self.size() * 2

    def __private_foo(self):
        print("private foo")

    def public_foo(self):
        self.__private_foo()


if __name__ == "__main__":
    foo = Foo([1, 2, 3])
    print(foo.nums)
    # print(foo.__size) # error
    print(foo.size())
    print(foo.double_size())
    # foo.__private_foo() # error
    foo.public_foo()
