#!/usr/bin/env python3

# Simple implementation of the Hashset.
class HashSet:
    def __init__(self, capacity=2):
        self._size = 0
        self._capacity = capacity
        self._array = [None for _ in range(capacity)]

    def insert(self, key):
        index = self._hash(key)
        while True:
            if self._array[index] == None:
                self._array[index] = key
                self._size += 1
                if self._size > self._capacity // 2:
                    self._rehash()
                return

            index += 1
            index %= self._capacity

    def remove(self, key):
        index = self._hash(key)

        while self._array[index] != None:
            if self._array[index] == key:
                self._array[index] = None
                self._size -= 1
            index += 1
            index %= self._capacity

    def get(self, key):
        index = self._hash(key)
        while self._array[index] != None:
            if self._array[index] == key:
                return self._array[index]
            index += 1
            index %= self._capacity

        return None

    def _hash(self, key):

        return key % self._capacity

    def _rehash(self):
        self._capacity = 2 * self._capacity
        tmp = self._array.copy()
        self._array = [None for _ in range(self._capacity)]
        self._size = 0
        for key in tmp:
            if key is not None:
                self.insert(key)


if __name__ == "__main__":
    hashmap = HashSet()
    hashmap.insert(1)
    hashmap.insert(2)
    print(hashmap.get(1))
    print(hashmap.get(2))
    hashmap.remove(1)
    print(hashmap.get(1))
    hashmap.insert(3)
    hashmap.insert(4)
    hashmap.insert(5)
    print(hashmap.get(3))
    print(hashmap.get(5))
