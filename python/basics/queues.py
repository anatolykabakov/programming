#!/usr/bin/env python3

if __name__ == "__main__":
    # using list as queue
    queue = []
    queue.append(1)
    queue.append(2)
    queue.append(3)
    print(queue)
    # remove from begin of queue
    queue.pop(0)
    print(queue)

    # double ended queue
    from collections import deque

    d = deque()
    d.append(1)
    d.append(2)
    d.append(3)
    print(d)
    d.popleft()  # remove element from begin of queue
    print(d)
    d.appendleft(0)
    print(d)
    d.pop()  # remove element from end of queue
    print(d)
