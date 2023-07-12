#!/usr/bin/env python3
import heapq


# Time O(ElogE) E -- edges number
def minimum_spanning_tree(graph, start):
    mst = []
    visit = set()
    visit.add(start)
    min_heap = []
    for v, w in graph[start]:
        heapq.heappush(min_heap, [w, start, v])

    while min_heap:
        _, u, v = heapq.heappop(min_heap)
        if v in visit:
            continue
        visit.add(v)
        mst.append([u, v])
        for neighbor, w2 in graph[v]:
            if neighbor not in visit:
                heapq.heappush(min_heap, [w2, v, neighbor])

    return mst


def build_graph(edges):
    graph = {}
    for u, v, w in edges:
        if u not in graph:
            graph[u] = []
        if v not in graph:
            graph[v] = []
        graph[u].append([v, w])
        graph[v].append([u, w])
    return graph


if __name__ == "__main__":
    # Q: Find the shortest path from source to destination node in graph.
    edges = [
        ["A", "B", 10],
        ["A", "C", 3],
        ["B", "D", 1],
        ["C", "B", 4],
        ["C", "D", 4],
        ["C", "E", 4],
        ["D", "E", 2],
    ]
    graph = build_graph(edges)
    print(minimum_spanning_tree(graph, "A"))
