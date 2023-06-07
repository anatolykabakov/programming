# /usr/bin/env python3


def find_node_lowest_cost(costs, processed):
    lowest_cost = 1000000
    lowest_node = None
    for node in costs.keys():
        cost = costs[node]
        if (cost < lowest_cost) and not (node in processed):
            lowest_cost = cost
            lowest_node = node
    return lowest_node


def dijkstra(graph, start, end):
    path = {}
    costs = {}
    processed = []
    neighbors = {}
    for node in graph.keys():
        cost = 1000000
        if node == start:
            cost = 0
        costs[node] = cost
        path[node] = []

    minimal_cost_node = start
    path[minimal_cost_node] = [start]

    while minimal_cost_node:
        if minimal_cost_node == end:
            break
        cost = costs[minimal_cost_node]
        neighbors = graph[minimal_cost_node]
        for node in neighbors.keys():
            new_cost = cost + neighbors[node]
            if new_cost < costs[node]:
                costs[node] = new_cost
                path[node] = path[minimal_cost_node] + [node]

        processed.append(minimal_cost_node)
        minimal_cost_node = find_node_lowest_cost(costs, processed)
    return path[end]


if __name__ == "__main__":
    graph = {
        "a": {"b": 2, "c": 1},
        "b": {"f": 7},
        "c": {"d": 5, "e": 2},
        "d": {"f": 2},
        "e": {"f": 1},
        "f": {"g": 1},
        "g": {},
    }
    result = dijkstra(graph, start="a", end="g")
    print(result)
