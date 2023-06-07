# https://leetcode.com/problems/sort-an-array/description/
from typing import List


class Solution:
    def sortArray(self, nums: List[int]) -> List[int]:
        return self.merge_sort(nums, 0, len(nums))

    def merge_sort(self, nums, l, r):
        if (r - l + 1) <= 1:
            return nums

        mid = l + (r - l) // 2

        self.merge_sort(nums, l, mid)
        self.merge_sort(nums, mid + 1, r)
        self.merge(nums, l, mid, r)

        return nums

    def merge(self, nums, l, m, r):
        left = nums[l : m + 1]
        right = nums[m + 1 : r + 1]

        k = l
        i = 0
        j = 0

        while i < len(left) and j < len(right):
            if left[i] <= right[j]:
                nums[k] = left[i]
                i += 1
            else:
                nums[k] = right[j]
                j += 1
            k += 1

        while i < len(left):
            nums[k] = left[i]
            k += 1
            i += 1

        while j < len(right):
            nums[k] = right[j]
            k += 1
            j += 1
