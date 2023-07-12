#!/usr/bin/env python3


class SegmentTree:
    def __init__(self, total, left_range_boundary, right_range_boundary):
        self.sum = total
        self.left_child = None
        self.right_child = None
        self.left_boundary = left_range_boundary
        self.right_boundary = right_range_boundary

    # Time O(n)
    @staticmethod
    def build(nums, left_boundary, right_boundary):
        if left_boundary == right_boundary:
            return SegmentTree(nums[left_boundary], left_boundary, right_boundary)

        mid = left_boundary + (right_boundary - left_boundary) // 2
        root = SegmentTree(0, left_boundary, right_boundary)
        root.left_child = SegmentTree.build(nums, left_boundary, mid)
        root.right_child = SegmentTree.build(nums, mid + 1, right_boundary)
        root.sum = root.left_child.sum + root.right_child.sum
        return root

    # Time O(logn)
    def update(self, index, val):
        if self.left_boundary == self.right_boundary:
            self.sum = val
            return
        mid = self.left_boundary + (self.right_boundary - self.left_boundary) // 2
        if index > mid:
            self.right_child.update(index, val)
        else:
            self.left_child.update(index, val)
        self.sum = self.left_child.sum + self.right_child.sum

    # Time O(logn)
    def range_query(self, left_boundary, right_boundary):
        if self.left_boundary == left_boundary and self.right_boundary == right_boundary:
            return self.sum
        mid = self.left_boundary + (self.right_boundary - self.left_boundary) // 2
        if left_boundary > mid:
            return self.right_child.range_query(left_boundary, right_boundary)
        elif right_boundary <= mid:
            return self.left_child.range_query(left_boundary, right_boundary)
        else:
            return self.right_child.range_query(
                left_boundary, mid
            ) + self.right_child.range_query(mid, right_boundary)


# Time O(n)
def brute_force(nums, left, right):
    total = 0
    for i in range(left, right + 1):
        total += nums[i]
    return total


if __name__ == "__main__":
    # Q: Suppose we were given a range of values. Then, given a left and a right pointer that defines the range, we want to be able to calculate the sum of the range
    nums = [5, 3, 7, 1, 4, 2]
    print(brute_force(nums, 0, 5))
    print(brute_force(nums, 0, 2))
    st = SegmentTree.build(nums, 0, len(nums) - 1)
    print(st.range_query(0, 5))
    print(st.range_query(0, 2))
