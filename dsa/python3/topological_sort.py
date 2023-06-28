#!/usr/bin/env python3
from collections import defaultdict


def topological_sort(edges):
    visit = set()
    path = set()
    nodes = []

    graph = defaultdict(list)
    for u, v in edges:
        graph[u] = []
        graph[v] = []

    for u, v in edges:
        graph[u].append(v)

    def dfs(node):
        if node in path:
            return False
        if node in visit:
            return True
        visit.add(node)
        path.add(node)
        for neighbor in graph[node]:
            dfs(neighbor)
        path.remove(node)
        nodes.append(node)
        return True

    for node in graph:
        if not dfs(node):
            return []
    nodes.reverse()
    return nodes


if __name__ == "__main__":
    # Q: Given a directed acyclical graph, return a valid topological ordering of the graph
    edges = [
        ["A", "C"],
        ["A", "B"],
        ["B", "D"],
        ["C", "E"],
        ["D", "F"],
        ["E", "F"],
    ]
    print(topological_sort(edges))
