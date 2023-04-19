"""
Поиск в ширину -- алгоритм поиска кратчайшего пути из вершины в вершину.
Описание:
1. Создаем таблицу стоимости пути из вершины в вершину
2. Создаем список вершин, которые обошли
3. Создаем список соседних вершин
4. Необходимо посчитать стоимость из текущей вершины в вершину графа
5. Найти ноду с минимальной стоимостью find_node_lowest_cost()
6. Записываем ноду ноду с минимальной стоимостью в список посещенных
7. Проходим по соседям ноды с минимальностью стоимостью
8. п.4-8


Сложность алгоритма O(n*n)
"""


def find_node_lowest_cost(costs, processed):
    """
    Функция для поиска минимальной стоимости
    """
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
    neighbord = {}
    for node in graph.keys():
        if node != start:
            cost = 1000000
        else:
            cost = 0
        costs[node] = cost
        path[node] = []

    minimal_cost_node = start
    path[minimal_cost_node] = [start]

    while minimal_cost_node:
        if minimal_cost_node == end:
            break
        cost = costs[minimal_cost_node]
        neighbord = graph[minimal_cost_node]
        for node in neighbord.keys():
            new_cost = cost + neighbord[node]
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
