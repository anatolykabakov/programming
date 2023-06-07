#!/usr/bin/env python3


class MinHeap:
    def __init__(self):
        self._heap = []
        self._heap.append(0)

    # Time O(logn)
    def push(self, value):
        self._heap.append(value)

        i = len(self._heap) - 1
        while i > 1 and self._heap[i] < self._heap[i // 2]:
            tmp = self._heap[i]
            self._heap[i] = self._heap[i // 2]
            self._heap[i // 2] = tmp
            i = i // 2

    # Time O(logn)
    def pop(self):
        if len(self._heap) == 1:
            return None

        # swap root and right most node of the last level of three
        min_node = self._heap[1]
        # remove last node
        self._heap[1] = self._heap.pop()
        # find where the tree's root is located
        i = 1
        while 2 * i < len(self._heap):
            if (
                len(self._heap) > 2 * i + 1
                and self._heap[i * 2 + 1] < self._heap[i * 2]
                and self._heap[i] > self._heap[2 * i + 1]
            ):

                tmp = self._heap[i]
                self._heap[i] = self._heap[i * 2 + 1]
                self._heap[i * 2 + 1] = tmp

                i = i * 2 + 1

            elif self._heap[i] > self._heap[i * 2]:

                tmp = self._heap[i]
                self._heap[i] = self._heap[2 * i]
                self._heap[i * 2] = tmp

                i = i * 2

            else:
                break

        return min_node

    def top(self):
        if len(self._heap) > 1:
            return self._heap[1]
        return None

    def get(self):
        return self._heap

    def heapify(self, array):
        array.append(array[0])

        self._heap = array
        self._heap[0] = 0
        cur = (len(self._heap) - 1) // 2
        while cur > 0:
            i = cur
            while 2 * i < len(self._heap):
                if (
                    2 * i + 1 < len(self._heap)
                    and self._heap[2 * i + 1] < self._heap[2 * i]
                    and self._heap[2 * i + 1] < self._heap[i]
                ):

                    tmp = self._heap[i]
                    self._heap[i] = self._heap[2 * i + 1]
                    self._heap[2 * i + 1] = tmp
                    i = 2 * i + 1
                elif self._heap[2 * i] < self._heap[i]:
                    tmp = self._heap[i]
                    self._heap[i] = self._heap[2 * i]
                    self._heap[2 * i] = tmp
                    i = 2 * i
                else:
                    break
            cur -= 1


if __name__ == "__main__":
    h = MinHeap()
    h.push(14)
    h.push(19)
    h.push(16)
    h.push(21)
    h.push(26)
    h.push(19)
    h.push(68)
    h.push(65)
    h.push(30)
    h.push(17)
    print(h.get())
    print(h.pop())
    print(h.get())

    h.heapify([60, 50, 80, 40, 10, 30, 70, 20, 90])
    print(h.get())
