"""
Граф -- это структура, которая представлена множеством вершин и набором ребер
(соединение между вершинами).

Очередь -- это структура данных, элементы добавляются в конец,
достаются из начала FIFO

Поиск в ширину -- алгоритм поиска пути из вершины в вершину
Описание:
1. Создаем очередь, заполняем стартовой точкой
2. Пока в очереди что -то есть
3. Извлекаем вершину из очереди
4. Если вершина не соединена с концом,
   то создаем новую очередь из текущей и списка вершин доступных в текущей
   иначе пусть из вершины в вершину существует
Сложность алгоритма O(n*n)
"""


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
