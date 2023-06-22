# /usr/bin/env python3

# Time: O(n)
def largest_sum_subarray(nums):
    if len(nums) == 0:
        return nums
    max_sum = nums[0]
    cur_sum = 0
    max_l, max_r = 0, 0
    l, r = 0, 0
    for r in range(len(nums)):
        if cur_sum < 0:
            cur_sum = 0
            l = r
        cur_sum += nums[r]
        if cur_sum > max_sum:
            max_l = l
            max_r = r
            max_sum = cur_sum
    return nums[max_l : max_r + 1]


# Time O(n)
def RLE(word):
    ans = ""
    l = 0
    while l < len(word):
        ans += word[l]
        r = l
        while r + 1 < len(word) and word[r] == word[r + 1]:
            r += 1
        if r - l > 1:
            ans += str(r - l + 1)
            l = r
        l += 1
    return ans


if __name__ == "__main__":
    # Q: return subarray containing largest sum
    print(largest_sum_subarray([4, -1, 2, -7, 3, 4]))
    print(largest_sum_subarray([4, -1]))
    print(largest_sum_subarray([4]))
    print(largest_sum_subarray([]))

    # Q: Given a string. Repeated characters must be replaced to number of repeated chars.
    print(RLE("AAAAABBBBZZZZ"))
    print(RLE("AAA"))
    print(RLE("ABCD"))
    print(RLE(""))
