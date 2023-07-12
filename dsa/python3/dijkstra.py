#!/usr/bin/env python3
import heapq

# Time O(n*n)
def shortest_path(graph, src, dst):
    costs = {w: 1000 for w in graph}
    paths = {w: [] for w in graph}
    visit = []
    costs[src] = 0
    paths[src] = [src]
    while src:
        visit.append(src)
        if src == dst:
            break

        if src not in graph:
            raise Exception("node not exist!")

        # Refresh the cost of neighbor nodes
        for node, w in graph[src]:
            new_cost = costs[src] + w
            if new_cost < costs[node] and node not in visit:
                costs[node] = new_cost
                paths[node] = paths[src] + [node]

        # Choose unvisited node with lowest cost
        min_cost_node = None
        min_cost = 1000
        for node, cost in costs.items():
            if min_cost > cost and node not in visit:
                min_cost = cost
                min_cost_node = node

        src = min_cost_node
    return paths[src]


# Time O(ElogE) E -- edges number
def shortest_path_optimized(graph, src, dst):
    min_heap = [[0, src, [src]]]
    paths = {}
    visit = []
    while min_heap:
        cost, node, path = heapq.heappop(min_heap)
        visit.append(node)
        paths[node] = path
        if node == dst:
            break

        if src not in graph:
            raise Exception("node not exist!")

        for neighbor, w in graph[node]:
            if neighbor not in visit:
                heapq.heappush(min_heap, [cost + w, neighbor, paths[node] + [neighbor]])
    return paths[dst]


def build_graph(edges):
    graph = {}
    for s, d, w in edges:
        if s not in graph:
            graph[s] = []
        if d not in graph:
            graph[d] = []
        graph[s].append([d, w])
    return graph


if __name__ == "__main__":
    # Q: Find the shortest path from source to destination node in graph.
    edges = [
        ["A", "B", 10],
        ["A", "C", 3],
        ["B", "D", 2],
        ["C", "B", 4],
        ["C", "D", 8],
        ["C", "E", 2],
        ["D", "E", 5],
    ]
    graph = build_graph(edges)
    print(shortest_path(graph, "A", "D"))  # ['A', 'C', 'B', 'D']
    print(shortest_path(graph, "C", "D"))  # ['C', 'B', 'D']
    print(shortest_path(graph, "D", "D"))  # ['D']
    print(shortest_path(graph, "A", "C"))  # ['A', 'C']
    try:
        print(shortest_path(graph, "AA", "CC"))
    except:
        print("oops")

    print(shortest_path_optimized(graph, "A", "D"))  # ['A', 'C', 'B', 'D']
    print(shortest_path_optimized(graph, "C", "D"))  # ['C', 'B', 'D']
    print(shortest_path_optimized(graph, "D", "D"))  # ['D']
    print(shortest_path_optimized(graph, "A", "C"))  # ['A', 'C']
    try:
        print(shortest_path(graph, "AA", "CC"))
    except:
        print("oops")
