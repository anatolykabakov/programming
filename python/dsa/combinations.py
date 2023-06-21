#!/usr/bin/env python3

# Time O(2^n)
def combinations(n, k):
    subsets = []

    def backtracking(i, subset):
        # base case
        if len(subset) == k:
            subsets.append(subset.copy())
            return
        if i > n:
            return

        # for each element, we explore all subsets that include i element
        subset.append(i)
        backtracking(i + 1, subset)
        # then we explowe subsets that dont not include i element
        subset.pop()
        backtracking(i + 1, subset)

    backtracking(0, [])

    return subsets


# Time O(n!/(k!(n - k)))
def combinations_optimized(n, k):
    subsets = []

    def backtracking(i, subset):
        # base case
        if len(subset) == k:
            subsets.append(subset.copy())
            return
        if i > n:
            return

        for j in range(i, n + 1):
            # for each element, we explore all subsets that include i element
            subset.append(j)
            backtracking(j + 1, subset)
            # then we explowe subsets that dont not include i element
            subset.pop()

    backtracking(0, [])

    return subsets


if __name__ == "__main__":
    # Q: Given two nums n and k, return all possible combinations of size = k, choosing from values between 1 and n.
    print(combinations(5, 2))
    print(combinations_optimized(5, 2))
