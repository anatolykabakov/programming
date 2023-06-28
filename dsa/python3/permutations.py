#!/usr/bin/env python3

# Time O(n * n * !n)
def permutations(nums):
    perm = [[]]
    for n in nums:
        new_perm = []
        for p in perm:
            for i in range(len(p) + 1):
                tmp = p.copy()
                tmp.insert(i, n)
                new_perm.append(tmp)
        perm = new_perm
    return perm


if __name__ == "__main__":
    # Q: Given a list of nums, return all possible distinct permutations of nums.
    print(permutations([1, 2, 3, 4]))
