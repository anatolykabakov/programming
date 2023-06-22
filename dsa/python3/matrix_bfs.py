#!/usr/bin/env python3

"""
Q: Find the length of the shortest path from top left of the grid to the bottom right.
"""
# Time O(N*M)
def bfs(matrix, r, c, visited):
    nrows, ncols = len(matrix), len(matrix[0])
    queue = []
    queue.append((r, c))
    length = 0
    while queue:
        for i in range(len(queue)):
            node = queue.pop(0)

            if node[0] == nrows - 1 and node[1] == ncols - 1:
                return length

            neighbors = [(0, 1), (0, -1), (1, 0), (-1, 0)]
            for dr, dc in neighbors:
                r, c = node[0] + dr, node[1] + dc
                if (
                    r < 0
                    or c < 0
                    or r == nrows
                    or c == ncols
                    or visited[r][c] == True
                    or matrix[r][c] == 1
                ):
                    continue
                visited[r][c] = True
                queue.append((r, c))
        length += 1
    return length


if __name__ == "__main__":
    matrix = [[0, 0, 0, 0], [1, 1, 0, 0], [0, 0, 0, 1], [0, 1, 0, 0]]
    visited = [[False] * len(matrix) for _ in range(len(matrix))]
    unique_paths = bfs(matrix, 0, 0, visited)
    print(unique_paths)
