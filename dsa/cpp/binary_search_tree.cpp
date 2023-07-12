#include <iostream>
#include <cassert>
#include <vector>

/**
 * @brief The binary search tree (BST) is data structure used to handle ordered distinct elements. Contains operations
 * insert, search and remove with time complexity O(logn) for balanced tree, in worst case O(n). A binary search tree
 * has property that for each node in the tree that has value, the value in the left child node must be smaller, the
 * value in the right child node must be greater.
 *
 */
class BinarySearchTree {
public:
  struct Node {
    Node* left;
    Node* right;
    int value;
    Node(int x) : value{x}, left{nullptr}, right{nullptr} {}
  };
  BinarySearchTree() : root_{nullptr} {}

  /**
   * @brief Method creates the Node with value and insert into BST.
   * If value is dublicated, false is returned.
   *
   * Time O(logn) Space O(1)
   *
   * @param value int value to be inserted.
   * @return bool true if inserted, false if the value is duplicated.
   */
  bool insert(int value)
  {
    if (root_ == nullptr) {
      root_ = new Node(value);
      return true;
    }

    Node* node = root_;
    Node* prev = nullptr;
    while (node) {
      // BST cannot have dublicate values.
      if (value == node->value) {
        return false;
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
    return true;
  }

  /**
   * @brief Method search the Node with value and remove it from BST.
   *
   * Time O(logn) Space O(logn)
   *
   * @param value int value to be removed.
   */
  void remove(int value) { remove_recursive_(root_, value); }

  /**
   * @brief Method searches the Node with value.
   *
   * Time O(logn) Space O(logn)
   *
   * @param value int value to be searched.
   * @return bool true if found, otherwise false.
   */
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

  /**
   * @brief Method returns the root Node of tree.
   *
   * @return Node* the root node.
   */
  Node* root() const { return root_; }

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
        Node* min_node = min_value_node_(root->right);
        root->value = min_node->value;
        root->right = remove_recursive_(root->right, min_node->value);
      }
    }
    return root;
  }

  Node* min_value_node_(Node* root) const
  {
    Node* cur = root;
    while (cur && cur->left) {
      cur = cur->left;
    }
    return cur;
  }
};

void print(const BinarySearchTree::Node* node)
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
  std::vector<int> nums = {4, 3, 6, 2, 5, 7};
  for (const auto& n : nums) {
    assert(bt.insert(n) == true);
  }
  assert(bt.insert(7) == false);

  print(bt.root());

  for (const auto& n : nums) {
    assert(bt.search(n) == true);
  }
  assert(bt.search(66) == false);

  bt.remove(4);
  bt.remove(3);
  bt.remove(6);

  assert(bt.search(4) == false);
  assert(bt.search(3) == false);
  assert(bt.search(6) == false);
  assert(bt.search(2) == true);
  assert(bt.search(5) == true);
  assert(bt.search(7) == true);

  print(bt.root());
  return 0;
}
