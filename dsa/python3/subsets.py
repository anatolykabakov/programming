#!/usr/bin/env python3

# Time O(2^n)
def distinct_subsets(nums):
    subsets = []

    def backtracking(i, subset):
        # base case
        if i >= len(nums):
            subsets.append(subset.copy())
            return
        # for each element, we explore all subsets that include i element
        subset.append(nums[i])
        backtracking(i + 1, subset)
        # then we explowe subsets that dont not include i element
        subset.pop()
        backtracking(i + 1, subset)

    backtracking(0, [])

    return subsets


# Time O(c^n)
def nondistinct_subsets(nums):
    subsets = []

    def backtracking(i, subset):
        # base case
        if i >= len(nums):
            subsets.append(subset.copy())
            return

        # for each element, we explore all subsets that include i element
        subset.append(nums[i])
        backtracking(i + 1, subset)
        # then we explowe subsets that dont not include i element
        subset.pop()
        # skip dublicates
        while i + 1 < len(nums) and nums[i] == nums[i + 1]:
            i += 1
        backtracking(i + 1, subset)

    nums.sort()
    backtracking(0, [])

    return subsets


if __name__ == "__main__":
    # Q: Given an integer array nums of unique int, return all possible distinct subsets.
    nums = [1, 2, 3]
    print(distinct_subsets(nums))

    # Q: Given an integer array nums that may contain duplicates, return all possible distinct subsets.
    nums = [1, 2, 2, 3, 3]
    print(nondistinct_subsets(nums))
