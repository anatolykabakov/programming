# Kadane's algorithm is a greedy/dynamic programming algorithm that
# can be used on array problems to bring the time complexity down to O(n)
# Question: Find non empty subarray with max sum. [2, 3, -1] = 6

# Time: O(n*n)
def brute_force(nums):
    max_sum = nums[0]
    for i in range(len(nums)):
        cur_sum = nums[i]
        for j in range(i + 1, len(nums)):
            cur_sum += nums[j]
            max_sum = max(max_sum, cur_sum)
    return max_sum


# Time: O(n)
def kadanes(nums):
    max_sum = nums[0]
    cur_sum = 0
    for n in nums:
        cur_sum = max(cur_sum, 0)
        cur_sum += n
        max_sum = max(max_sum, cur_sum)
    return max_sum


if __name__ == "__main__":
    nums = [4, -1, 2, -7, 3, 4]
    answer = brute_force(nums)
    print(answer)
    answer = kadanes(nums)
    print(answer)
