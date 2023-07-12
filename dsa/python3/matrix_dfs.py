#!/usr/bin/env python3

# Time O(4^N*M)
def dfs(matrix, r, c, visited):
    nrows, ncols = len(matrix), len(matrix[0])
    if (
        r == nrows
        or c == ncols
        or r < 0
        or c < 0
        or matrix[r][c] == 1
        or visited[r][c] == True
    ):
        return 0
    if r == nrows - 1 and c == ncols - 1:
        return 1

    visited[r][c] = True

    count = 0
    count += dfs(matrix, r, c + 1, visited)
    count += dfs(matrix, r, c - 1, visited)
    count += dfs(matrix, r + 1, c, visited)
    count += dfs(matrix, r - 1, c, visited)

    visited[r][c] = False

    return count


if __name__ == "__main__":
    # Q: Count the unique paths from the top left to the bottom right.
    # A single path may only move along 0's and can't visit the same cell more than once.
    matrix = [[0, 0, 0, 0], [1, 1, 0, 0], [0, 0, 0, 1], [0, 1, 0, 0]]
    visited = [[False] * len(matrix) for _ in range(len(matrix))]
    unique_paths = dfs(matrix, 0, 0, visited)
    print(unique_paths)
