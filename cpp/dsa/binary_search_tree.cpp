#include <iostream>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int x) : value{x}, left{nullptr}, right{nullptr} {}
};

class BinarySearchTree {
public:
  BinarySearchTree() : root_{nullptr} {}

  // O(logn)
  void insert(int value)
  {
    if (root_ == nullptr) {
      root_ = new Node(value);
      return;
    }

    Node* node = root_;
    Node* prev = nullptr;
    while (node) {
      // BST cannot have dublicate values.
      if (value == node->value) {
        return;
      }
      prev = node;
      if (value > node->value) {
        node = node->right;
      } else {
        node = node->left;
      }
    }

    if (prev->value > value) {
      prev->left = new Node(value);
    } else {
      prev->right = new Node(value);
    }
  }

  Node* min_value_node(Node* root) const
  {
    Node* cur = root;
    while (cur && cur->left) {
      cur = cur->left;
    }
    return cur;
  }

  void remove(int value) { remove_recursive_(root_, value); }

  // O(logn)
  bool search(int value) const
  {
    if (root_ == nullptr) {
      return false;
    }
    Node* node = root_;
    while (node) {
      if (node->value == value) {
        return true;
      }
      if (node->value < value) {
        node = node->right;
      } else {
        node = node->left;
      }
    }
    return false;
  }

  Node* root() { return root_; }

private:
  Node* root_;

private:
  Node* remove_recursive_(Node* root, int value)
  {
    if (root == nullptr) {
      return nullptr;
    }

    if (root->value > value) {
      root->left = remove_recursive_(root->left, value);
    } else if (root->value < value) {
      root->right = remove_recursive_(root->right, value);
    } else {
      if (!root->left) {
        return root->right;
      } else if (!root->right) {
        return root->left;
      } else {
        Node* min_node = min_value_node(root->right);
        root->value = min_node->value;
        root->right = remove_recursive_(root->right, min_node->value);
      }
    }
    return root;
  }
};

void print(const Node* node)
{
  if (node == nullptr) {
    return;
  }
  std::cout << node->value << std::endl;
  print(node->left);
  print(node->right);
}

int main()
{
  BinarySearchTree bt;
  bt.insert(4);
  bt.insert(3);
  bt.insert(6);
  bt.insert(2);
  bt.insert(5);
  bt.insert(7);
  print(bt.root());
  std::cout << bt.search(6) << std::endl;
  std::cout << bt.search(66) << std::endl;
  bt.remove(4);
  bt.remove(3);
  bt.remove(2);
  print(bt.root());
  return 0;
}
