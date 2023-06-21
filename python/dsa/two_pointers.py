#!/usr/bin/env python3

"""
Two pointers pattern: We will start left pointer at index 0 and right pointer at len(nums) - 1,
increment either the L, or decrement R or both depending on the conditions given in the problem.
This repeats until the pointers meet each other.
"""

# Time O(n)
def is_palindrome(word):
    l, r = 0, len(word) - 1
    while l < r:
        if word[l] != word[r]:
            return False
        l += 1
        r -= 1
    return True


# Time O(n)
def find_sum(nums, target):
    l, r = 0, len(nums) - 1

    while l < r:
        cur_sum = nums[l] + nums[r]
        if cur_sum > target:
            r -= 1
        elif cur_sum < target:
            l += 1
        else:
            return [l, r]
    return []


# Time O(n*n)
def find_sum_brute_force(nums, target):
    for i in range(len(nums)):
        for j in range(i, len(nums)):
            cur_sum = nums[i] + nums[j]
            if cur_sum == target:
                return [i, j]
    return []


if __name__ == "__main__":
    # Q: Check if word is palindrome
    print(is_palindrome("aba"))  # True
    print(is_palindrome("abb"))  # False
    print(is_palindrome("abba"))  # True

    # Q: Given a sorted input array, return the two indices
    # of two elements which sums up to the target value.
    # Assume there's exactly one solution.
    nums = [-1, 2, 4, 3, 7, 9]
    # brute force approach
    print(find_sum_brute_force(nums, 7))  # [2, 3]
    print(find_sum_brute_force(nums, 77))  # []
    # two pointers is optimization for brute force approach
    print(find_sum(nums, 7))  # [2, 3]
    print(find_sum(nums, 77))  # []
