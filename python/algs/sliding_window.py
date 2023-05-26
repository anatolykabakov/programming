# return subarray containing largest sum

# Time: O(n)
def sliding_window(nums):
    max_sum = nums[0]
    cur_sum = 0
    max_l, max_r = 0, 0
    l, r = 0, 0
    for i in range(len(nums)):
        r = i
        if cur_sum < 0:
            cur_sum = 0
            l = r
        cur_sum += nums[i]
        if cur_sum > max_sum:
            max_l = l
            max_r = r
            max_sum = cur_sum
    return nums[max_l : max_r + 1]


if __name__ == "__main__":
    nums = [4, -1, 2, -7, 3, 4]
    answer = sliding_window(nums)
    print(answer)
