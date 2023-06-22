"""
It was necessary to compute the sum of all node values
"""

tree = [
    {
        "v": 5,
        "c": [
            {
                "v": 10,
                "c": [
                    {
                        "v": 11,
                    }
                ],
            },
            {"v": 7, "c": [{"v": 5, "c": [{"v": 1}]}]},
        ],
    },
    {"v": 5, "c": [{"v": 10}, {"v": 15}]},
]


def sum_of_tree_recursive(tree):
    result = 0

    for node in tree:
        result += node["v"]
        if "c" in node:
            result += sum_of_tree_recursive(node["c"])

    return result


def sum_of_tree(tree):
    result = 0
    stack = []

    for node in tree:
        stack.append(node)

    while len(stack) > 0:
        node = stack.pop()
        result += node["v"]
        if "c" in node:
            for child in node["c"]:
                stack.append(child)
    return result


if __name__ == "__main__":
    result = sum_of_tree_recursive(tree)
    print(result)
    result = sum_of_tree(tree)
    print(result)
