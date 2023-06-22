#!/usr/bin/env python3


class Node:
    def __init__(self, value, left=None, right=None):
        self.right = right
        self.left = left
        self.value = value


class BST:
    def __init__(self):
        self.__root = None

    def insert(self, value):
        self.__root = self._insert_recursive(self.__root, value)

    def remove(self, value):
        self.__root = self._remove_recursive(self.__root, value)

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

    def root(self):
        return self.__root


def print_bst(root):
    if root is None:
        return
    print(root.value)
    print_bst(root.left)
    print_bst(root.right)


if __name__ == "__main__":
    bst = BST()
    bst.insert(4)
    bst.insert(3)
    bst.insert(5)
    bst.insert(2)
    print_bst(bst.root())
    bst.remove(4)
    bst.remove(3)
    bst.remove(2)
    print_bst(bst.root())
