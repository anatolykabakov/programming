#!/usr/bin/env python3


class Pair:
    def __init__(self, key, value):
        self.key, self.value = key, value


# Simple implementation of the Hashmap.
# Using the open-address approach to avoid collisions.
class HashMap:
    def __init__(self, capacity=2):
        self._size = 0
        self._capacity = capacity
        self._array = [None for _ in range(capacity)]

    def insert(self, key, value):
        index = self._hash(key)
        while True:
            if self._array[index] == None:
                self._array[index] = Pair(key, value)
                self._size += 1
                if self._size > self._capacity // 2:
                    self._rehash()
                return
            elif self._array[index].key == key:
                self._array[index].value = value
                return

            index += 1
            index %= self._capacity

    def remove(self, key):
        index = self._hash(key)

        while self._array[index] != None:
            if self._array[index].key == key:
                self._array[index] = None
                self._size -= 1
            index += 1
            index %= self._capacity

    def get(self, key):
        index = self._hash(key)
        while self._array[index] != None:
            if self._array[index].key == key:
                return self._array[index].value
            index += 1
            index %= self._capacity

        return None

    def _hash(self, key):
        index = 0
        for c in key:
            index += ord(c)
        return index % self._capacity

    def _rehash(self):
        self._capacity = 2 * self._capacity
        tmp = self._array.copy()
        self._array = [None for _ in range(self._capacity)]
        self._size = 0
        for pair in tmp:
            if pair is not None:
                self.insert(pair.key, pair.value)


if __name__ == "__main__":
    hashmap = HashMap()
    hashmap.insert("1", 1)
    hashmap.insert("2", 2)
    print(hashmap.get("1"))
    print(hashmap.get("2"))
    hashmap.remove("1")
    print(hashmap.get("1"))
    hashmap.insert("3", 3)
    hashmap.insert("4", 3)
    hashmap.insert("5", 3)
    print(hashmap.get("3"))
