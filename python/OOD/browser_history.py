# ref https://leetcode.com/problems/design-browser-history/description/


class Node:
    def __init__(self, value):
        self.value = value
        self.next = None
        self.prev = None


class BrowserHistory:
    def __init__(self, homepage: str):
        self.curr = Node(homepage)

    def visit(self, url: str) -> None:
        node = Node(url)
        node.prev = self.curr
        node.next = None
        self.curr.next = node
        self.curr = node

    def back(self, steps: int) -> str:
        while steps > 0 and self.curr.prev:
            self.curr = self.curr.prev
            steps -= 1
        return self.curr.value

    def forward(self, steps: int) -> str:
        while steps > 0 and self.curr.next:
            self.curr = self.curr.next
            steps -= 1
        return self.curr.value


if __name__ == "__main__":
    browser_history = BrowserHistory("leetcode.com")
    browser_history.visit("google.com")
    print(browser_history.back(1))
    print(browser_history.forward(1))
