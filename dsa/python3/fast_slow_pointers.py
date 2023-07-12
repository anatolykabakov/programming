#!/usr/bin/env python3


class Node:
    def __init__(self, val, next=None):
        self.val, self.next = val, next


# Time O(n)
def middle_of_list_brute_force(root):
    length = 0
    tmp = root
    while tmp:
        length += 1
        tmp = tmp.next

    cur = 0
    tmp = root
    while tmp and cur != length // 2:
        cur += 1
        tmp = tmp.next
    return tmp


# Time O(n)
def middle_of_list_fast_slow_pointers(root):
    slow, fast = root, root
    while fast and fast.next:
        slow = slow.next
        fast = fast.next.next
    return slow


# Time O(n) Space O(n)
def has_cycle_hashmap(root):
    visit = set()
    while root:
        if root in visit:
            return True
        visit.add(root)
        root = root.next
    return False


# Time O(n) Space O(1)
def has_cycle_fast_slow_pointers(root):
    slow, fast = root, root
    while fast and fast.next:
        slow = slow.next
        fast = fast.next.next
        if slow.val == fast.val:
            return True
    return False


def floyds_tortoise_and_hare_alghorihm(root):
    tortoise, hare = root, root
    while hare and hare.next:
        tortoise = tortoise.next
        hare = hare.next.next
        if tortoise == hare:
            break

    if hare is None or hare.next is None:
        return None

    slow = root
    while slow != tortoise:
        slow = slow.next
        tortoise = tortoise.next
    return slow


if __name__ == "__main__":
    # Q: Find the middle of a linked list.
    root = Node(1)
    root.next = Node(2)
    root.next.next = Node(3)
    root.next.next.next = Node(4)
    root.next.next.next.next = Node(5)

    middle = middle_of_list_brute_force(root)
    print(middle.val)  # 3

    middle = middle_of_list_fast_slow_pointers(root)
    print(middle.val)  # 3

    # Q: Determine if a Linked List has a cycle.

    print(has_cycle_hashmap(root))  # False
    print(has_cycle_fast_slow_pointers(root))  # False

    root.next.next.next.next.next = root.next

    print(has_cycle_hashmap(root))  # True
    print(has_cycle_fast_slow_pointers(root))  # True

    # Q: Determine if a Linked List has a cycle and return the head.
    cycle_begin = floyds_tortoise_and_hare_alghorihm(root)
    print(cycle_begin.val)
