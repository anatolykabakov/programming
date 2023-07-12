#!/usr/bin/env python3


class Node:
    def __init__(self, value):
        self.value = value
        self.neighbors = []


def dfs(graph, src, dst, visit):
    if src in visit:
        return 0
    if src == dst:
        return 1

    visit.add(src)

    count = 0
    for node in graph[src]:
        count += dfs(graph, node, dst, visit)

    visit.remove(src)
    return count


def bfs(graph, src, dst, visit):
    queue = []
    queue.append(src)
    visit.add(src)
    shortest_path = 0
    while queue:
        for i in range(len(queue)):
            node = queue.pop(0)
            if node == dst:
                return shortest_path
            for neighbor in graph[node]:
                queue.append(neighbor)
                visit.add(neighbor)
        shortest_path += 1


if __name__ == "__main__":
    # Build the graph
    edges = [["A", "B"], ["B", "C"], ["B", "E"], ["C", "E"], ["E", "D"]]
    adj_list = {}
    for src, dst in edges:
        if src not in adj_list:
            adj_list[src] = []
        if dst not in adj_list:
            adj_list[dst] = []
        adj_list[src].append(dst)
    # Find the numbers of unique paths
    print(dfs(adj_list, "A", "D", set()))
    # Find shortest path
    print(dfs(adj_list, "A", "D", set()))
