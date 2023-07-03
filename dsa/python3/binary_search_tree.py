#!/usr/bin/env python3
import unittest


class Node:
    def __init__(self, value, left=None, right=None):
        self.right = right
        self.left = left
        self.value = value


class BST:
    """
    The binary search tree (BST) is data structure used to handle ordered distinct elements. Contains operations
    insert, search and remove with time complexity O(logn) for balanced tree, in worst case O(n). A binary search tree
    has property that for each node in the tree that has value, the value in the left child node must be smaller, the
    value in the right child node must be greater.
    """

    def __init__(self, nums=[]):
        self._root = None
        for num in nums:
            self.insert(num)

    """
    Method creates the Node with value and insert into BST.
    If value is dublicated, false is returned.
    """

    def insert(self, value):
        self._root = self._insert_recursive(self._root, value)

    """
    Method search the Node with value and remove it from BST.
    """

    def remove(self, value):
        self._root = self._remove_recursive(self._root, value)

    def search(self, value):
        node = self._search_recursive(self._root, value)
        return True if node is not None else False

    def _search_recursive(self, root, value):
        if root is None:
            return None
        if root.value < value:
            return self._search_recursive(root.right, value)
        elif root.value > value:
            return self._search_recursive(root.left, value)
        return root

    def _remove_recursive(self, root, value):
        if root is None:
            return None
        if value > root.value:
            root.right = self._remove_recursive(root.right, value)
        elif value < root.value:
            root.left = self._remove_recursive(root.left, value)
        else:
            if not root.left:
                return root.right
            elif not root.right:
                return root.left
            else:
                node = self.min_node(root.right)
                root.value = node.value
                root.right = self._remove_recursive(root.right, node.value)
        return root

    def min_node(self, root):
        cur = root
        while cur and cur.left:
            cur = cur.left
        return cur

    def _insert_recursive(self, root, value):
        if root is None:
            return Node(value)
        if value > root.value:
            root.right = self._insert_recursive(root.right, value)
        elif value < root.value:
            root.left = self._insert_recursive(root.left, value)
        return root

    """
    Returns the root of tree.
    """

    def root(self):
        return self._root


def print_bst(root):
    if root is None:
        return
    print(root.value)
    print_bst(root.left)
    print_bst(root.right)


class BSTTest(unittest.TestCase):
    def setUp(self):
        self.testdata = [4, 3, 6, 2, 5, 7]
        self.bst = BST(self.testdata)

    def test_insert(self):
        self.bst.insert(8)
        self.assertTrue(self.bst.search(8))

    def test_search(self):
        for num in self.testdata:
            self.assertTrue(self.bst.search(num))

        self.assertFalse(self.bst.search(88))

    def test_remove(self):
        self.bst.remove(4)
        self.bst.remove(3)
        self.bst.remove(2)

        self.assertFalse(self.bst.search(4))
        self.assertFalse(self.bst.search(3))
        self.assertFalse(self.bst.search(2))

        self.assertTrue(self.bst.search(6))
        self.assertTrue(self.bst.search(5))
        self.assertTrue(self.bst.search(7))


if __name__ == "__main__":
    bst = BST([4, 3, 6, 2, 5, 7])
    print_bst(bst.root())

    unittest.main()
