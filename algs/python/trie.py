"""

This is the implementation of Trie

A trie (pronounced as "try") or prefix tree is a tree data structure
used to efficiently store
and retrieve keys in a dataset of strings.
There are various applications of this data structure,
such as autocomplete and spellchecker.

"""


class Node:
    def __init__(self):
        self.end = False
        self.childrens = {}


class Trie:
    def __init__(self):
        self.root = Node()

    def insert(self, word: str) -> None:
        cur = self.root
        for c in word:
            if c not in cur.childrens:
                cur.childrens[c] = Node()
            cur = cur.childrens[c]
        cur.end = True

    def search(self, word: str) -> bool:
        cur = self.root
        for c in word:
            if c not in cur.childrens:
                return False
            cur = cur.childrens[c]
        return cur.end

    def startsWith(self, prefix: str) -> bool:
        cur = self.root
        for c in prefix:
            if c not in cur.childrens:
                return False
            cur = cur.childrens[c]
        return True


if __name__ == "__main__":
    trie = Trie()
    trie.insert("apple")
    print(trie.search("apple"))  # True
    print(trie.startsWith("app"))  # True
    print(trie.search("b"))  # False
    print(trie.startsWith("b"))  # False
