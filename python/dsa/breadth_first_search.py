# /usr/bin/env python3


def breadth_first_search(graph, start, end):
    quene = []
    quene.append(start)
    path = []

    while len(quene) > 0:
        current = quene.pop()
        path.append(current)
        if not (graph[current]):
            graph[current] = []
        if end in graph[current]:
            path.append(end)
            return path
        else:
            quene = quene + graph[current]

    return None


if __name__ == "__main__":
    graph = {
        "a": ["b", "c"],
        "b": ["f"],
        "c": ["d", "e"],
        "d": ["f"],
        "e": ["f"],
        "f": ["g"],
    }
    result = breadth_first_search(graph, start="a", end="g")
    print(result)
