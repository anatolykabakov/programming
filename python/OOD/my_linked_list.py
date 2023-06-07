"""
LeetCode 707. Design Linked List

Design your implementation of the linked list. You can choose to use a singly or doubly linked list.
A node in a singly linked list should have two attributes: val and next. val is the value of the current node, and next is a pointer/reference to the next node.
If you want to use the doubly linked list, you will need one more attribute prev to indicate the previous node in the linked list. Assume all nodes in the linked list are 0-indexed.

Implement the MyLinkedList class:

MyLinkedList() Initializes the MyLinkedList object.
int get(int index) Get the value of the indexth node in the linked list. If the index is invalid, return -1.
void addAtHead(int val) Add a node of value val before the first element of the linked list. After the insertion, the new node will be the first node of the linked list.
void addAtTail(int val) Append a node of value val as the last element of the linked list.
void addAtIndex(int index, int val) Add a node of value val before the indexth node in the linked list. If index equals the length of the linked list, the node will be appended to the end of the linked list. If index is greater than the length, the node will not be inserted.
void deleteAtIndex(int index) Delete the indexth node in the linked list, if the index is valid.

"""


class Node:
    def __init__(self, val):
        self.val = val
        self.next = None
        self.prev = None


class MyLinkedList:
    def __init__(self):
        self.head = Node(-1)
        self.tail = Node(-1)
        self.tail.prev = self.head
        self.head.next = self.tail

    def get(self, index: int) -> int:
        i = 0
        cur = self.head.next
        while i < index and cur != None:
            i += 1
            cur = cur.next
        if cur and cur != self.tail and i == index:
            return cur.val
        return -1

    def addAtHead(self, val: int) -> None:
        node = Node(val)
        node.prev = self.head
        node.next = self.head.next

        self.head.next.prev = node
        self.head.next = node

    def addAtTail(self, val: int) -> None:
        node = Node(val)
        node.prev = self.tail.prev
        node.next = self.tail

        self.tail.prev.next = node
        self.tail.prev = node

    def addAtIndex(self, index: int, val: int) -> None:
        i = 0
        cur = self.head.next
        while i < index and cur != None:
            i += 1
            cur = cur.next
        if cur and i == index:
            node = Node(val)
            node.prev = cur.prev
            node.next = cur

            cur.prev.next = node
            cur.prev = node

    def deleteAtIndex(self, index: int) -> None:
        i = 0
        cur = self.head.next
        while i < index and cur != None:
            i += 1
            cur = cur.next
        if cur and i == index and cur != self.tail:
            cur.prev.next = cur.next
            cur.next.prev = cur.prev


if __name__ == "__main__":
    # Your MyLinkedList object will be instantiated and called as such:
    myLinkedList = MyLinkedList()
    myLinkedList.addAtHead(1)
    myLinkedList.addAtTail(3)
    myLinkedList.addAtIndex(1, 2)  # linked list becomes 1->2->3
    print(myLinkedList.get(1))  # return 2
    myLinkedList.deleteAtIndex(1)  # now the linked list is 1->3
    print(myLinkedList.get(1))  # return 3
