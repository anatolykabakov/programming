# /usr/bin/env python3
import unittest


class Node:
    def __init__(self, value, left=None, right=None):
        self.val = value
        self.left = left
        self.right = right


def breadth_first_search(root):
    """
    The bfs algorithm traverse the tree, level by level,
    and stores the node values at each depth level.
    """
    quene = []
    if root:
        quene.append(root)
    lvls = []

    while len(quene) > 0:
        lvl = []
        size = len(quene)
        for _ in range(size):
            cur = quene.pop(0)
            lvl.append(cur.val)
            if cur.left:
                quene.append(cur.left)
            if cur.right:
                quene.append(cur.right)

        lvls.append(lvl)
    return lvls


class BreadthFirstSearchTest(unittest.TestCase):
    def test_default(self):
        root = Node(4)
        root.left = Node(3)
        root.right = Node(6)
        root.left.right = Node(2)
        root.right.left = Node(5)
        root.right.right = Node(7)

        result = breadth_first_search(root)
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0], [4])
        self.assertEqual(result[1], [3, 6])
        self.assertEqual(result[2], [2, 5, 7])

    def test_invalid(self):
        result = breadth_first_search(None)
        self.assertEqual(result, [])


if __name__ == "__main__":
    unittest.main()
