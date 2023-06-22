#!/usr/bin/env python3

# https://leetcode.com/problems/range-sum-query-immutable/

from typing import List


class NumArray:
    def __init__(self, nums: List[int]):
        self.nums = nums
        self.prefixsum = []
        total = 0
        for n in nums:
            total += n
            self.prefixsum.append(total)

    # Time O(1)
    def sumRange(self, left: int, right: int) -> int:
        right = self.prefixsum[right]
        left = self.prefixsum[left - 1] if left > 0 else 0
        return right - left


if __name__ == "__main__":
    # Your NumArray object will be instantiated and called as such:
    nums = [-2, 0, 3, -5, 2, -1]
    obj = NumArray(nums)
    print(obj.sumRange(0, 2))
    print(obj.sumRange(2, 5))
    print(obj.sumRange(0, 5))
