"""
a* -- алгоритм поиска кратчайшего пути в графе

ref https://www.101computing.net/a-star-search-algorithm/
"""


def find_minimal_cost_node(costs, visited):
    minimal_cost = 10000
    minimal_cost_node = ""
    for key in costs.keys():
        if (costs[key]["total_dist"] < minimal_cost) and not (key in visited):
            minimal_cost = costs[key]["total_dist"]
            minimal_cost_node = key

    return minimal_cost_node


def astar(graph, start, end):
    costs = {}
    visited = []

    for node in graph.keys():
        costs[node] = {}
        costs[node]["short_dist"] = 10000
        costs[node]["hueristic_dist"] = 10000
        costs[node]["total_dist"] = 10000
        costs[node]["prev_node"] = ""
    costs[start]["short_dist"] = 0
    costs[start]["hueristic_dist"] = 14
    costs[start]["total_dist"] = 14
    minimal_cost_node = find_minimal_cost_node(costs, visited)

    while minimal_cost_node != end:
        for node in graph[minimal_cost_node].keys():
            hueristic_dist = graph[minimal_cost_node][node]["h"]
            short_dist = (
                costs[minimal_cost_node]["short_dist"]
                + graph[minimal_cost_node][node]["g"]
            )
            total_dist = short_dist + hueristic_dist
            if costs[node]["short_dist"] > short_dist:
                costs[node]["short_dist"] = short_dist
                costs[node]["total_dist"] = total_dist
                costs[node]["hueristic_dist"] = hueristic_dist
                costs[node]["prev_node"] = minimal_cost_node

        minimal_cost_node = find_minimal_cost_node(costs, visited)
        visited.append(minimal_cost_node)

    path = []
    path.append(end)
    node = end
    while node != start:
        path.append(costs[node]["prev_node"])
        node = costs[node]["prev_node"]
    path.reverse()
    return path


if __name__ == "__main__":
    graph = {
        "a": {"b": {"g": 4, "h": 12}, "c": {"g": 3, "h": 11}},
        "b": {"f": {"g": 5, "h": 11}, "e": {"g": 12, "h": 4}},
        "c": {"e": {"g": 10, "h": 4}, "d": {"g": 7, "h": 6}},
        "d": {
            "e": {"g": 2, "h": 4},
        },
        "e": {
            "z": {"g": 5, "h": 0},
        },
        "f": {
            "z": {"g": 16, "h": 0},
        },
        "z": {
            "z": {"g": 0, "h": 0},
        },
    }
    path = astar(graph, start="a", end="z")
    print(path)
