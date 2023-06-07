#!/usr/bin/env python3
import heapq

if __name__ == "__main__":
    min_heap = []
    heapq.heappush(min_heap, 3)
    heapq.heappush(min_heap, 2)
    heapq.heappush(min_heap, 4)
    print(min_heap[0])
    print(min_heap)

    max_heap = []
    # No max heaps by deault. Use multiply by -1 when push & pop
    heapq.heappush(max_heap, -3)
    heapq.heappush(max_heap, -2)
    heapq.heappush(max_heap, -4)
    print(-1 * max_heap[0])
    print(max_heap)

    # build heap from list
    nums = [2, 1, 8, 4, 5]
    max_heap = []
    for n in nums:
        heapq.heappush(max_heap, -n)
    print(nums)
    while max_heap:
        print(-1 * heapq.heappop(max_heap))
    # heapq.heapify(nums)
