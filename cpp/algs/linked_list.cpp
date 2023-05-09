#include <iostream>

struct Node {
    Node *next;
    int value;
    Node(int x) : value{x}, next{nullptr} {}
};

class LinkedList {
public:
    LinkedList() : size_(0), root_{nullptr} {}

    void add(int value) {
        if (root_ == nullptr) {
            root_ = new Node(value);
            size_ = +1;
            return;
        }

        Node *node = root_;
        while (node->next != nullptr) {
            node = node->next;
        }

        node->next = new Node(value);
        size_ += 1;
    }

    int size() { return size_; }

    void print() {
        Node *node = root_;
        std::cout << node->value << std::endl;
        while (node->next != nullptr) {
            node = node->next;
            std::cout << node->value << std::endl;
        }
    }

private:
    Node *root_;
    int size_;
};

int main() {
    LinkedList ll;
    ll.add(0);
    ll.add(1);
    ll.add(2);
    ll.add(3);
    ll.print();

    return 0;
}
