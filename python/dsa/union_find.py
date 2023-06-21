#!/usr/bin/env python3

# Time O(logN)
class UnionFind:
    def __init__(self, n):
        self.parent = {}  # key is the vertex and the value is the parent
        self.rank = {}

        for i in range(1, n + 1):
            self.parent[i] = i
            self.rank[i] = 1

    # takes vertex n and return its parent
    def find(self, n):
        p = self.parent[n]
        while p != self.parent[p]:
            # perform the path compression
            self.parent[p] = self.parent[self.parent[p]]
            p = self.parent[p]
        return p

    # takes two vertices and determines if a union can be performed
    def union(self, n1, n2):
        p1, p2 = self.find(n1), self.find(n2)
        if p1 == p2:
            return False

        if self.rank[p1] > self.rank[p2]:
            self.parent[p2] = p1
            self.rank[p1] += self.rank[p2]
        else:
            self.parent[p1] = p2
            self.rank[p2] += self.rank[p1]
        return True


# Time O(logN) Space O(N)
def detect_cycle_union_find(edges):
    uf = UnionFind(len(edges) + 1)

    for u, v in edges:
        if not uf.union(u, v):
            return True
    return False


# Time O(N*N) Space O(N)
def detect_cycle_dfs(edges):
    graph = {}

    def dfs(src, tgt, visit):
        if src in visit:
            return False

        visit.add(src)

        if src == tgt:
            return True

        for neighbor in graph[src]:
            if dfs(neighbor, tgt, visit):
                return True
        return False

    for u, v in edges:
        if u in graph and v in graph and dfs(u, v, set()):
            return True
        if u not in graph:
            graph[u] = []
        if v not in graph:
            graph[v] = []

        graph[u].append(v)
        graph[v].append(u)
    return False


if __name__ == "__main__":
    # Q: Given an undirected graph, the task is to check if the graph contains a cycle or not.
    # https://leetcode.com/problems/redundant-connection/description/
    edges = [[1, 2], [1, 3]]
    print(detect_cycle_dfs(edges))  # False
    print(detect_cycle_union_find(edges))  # False
    edges.append([2, 3])
    print(detect_cycle_dfs(edges))  # True
    print(detect_cycle_union_find(edges))  # True
