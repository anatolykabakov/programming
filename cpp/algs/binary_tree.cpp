#include <iostream>
// O(log2n)

struct Node {
    Node *left;
    Node *right;
    int value;
    Node(int x) : value{x}, left{nullptr}, right{nullptr} {}
};

class BinaryTree {
public:
    BinaryTree() : root_{nullptr} {}

    void add(int value) {
        if (root_ == nullptr) {
            root_ = new Node(value);
            return;
        }

        Node *node = root_;

        while (node) {
            if (value > node->value && node->right) {
                node = node->right;
            } else if (value < node->value && node->left) {
                node = node->left;
            } else {
                break;
            }
        }

        if (value > node->value) {
            node->right = new Node(value);
        } else {
            node->left = new Node(value);
        }
    }

    Node *root() { return root_; }

    void print(Node *node) {
        if (node == nullptr) {
            return;
        }
        std::cout << node->value << std::endl;
        print(node->left);
        print(node->right);
    }

private:
    Node *root_;
};

int main() {
    BinaryTree bt;
    bt.add(5);
    bt.add(2);
    bt.add(6);
    bt.add(2);
    bt.add(1);
    bt.print(bt.root());
    return 0;
}
