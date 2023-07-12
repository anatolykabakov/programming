#include <iostream>
#include <queue>
#include <cassert>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int value) : value{value}, left{nullptr}, right{nullptr} {};
};

/**
 * @brief The function traverses the tree and stores node values at each level.
 * Breadth first search alghorithm is implemented.
 *
 * @param root The root of binary tree
 * @return Node values at each level
 */
std::vector<std::vector<int>> breadth_first_search(Node* root)
{
  std::queue<Node*> q;
  if (root) {
    q.push(root);
  }
  std::vector<std::vector<int>> levels;
  int depth = 0;
  while (!q.empty()) {
    std::vector<int> level;
    int size = q.size();
    for (uint i = 0; i < size; ++i) {
      Node* cur = q.front();
      q.pop();
      level.push_back(cur->value);
      if (cur->left) {
        q.push(cur->left);
      }
      if (cur->right) {
        q.push(cur->right);
      }
    }
    levels.push_back(level);
    ++depth;
  }
  return levels;
}

int main()
{
  // Q : A binary tree is given. For each level, you must return node values.

  Node* root{nullptr};
  root = new Node(4);
  root->left = new Node(3);
  root->right = new Node(6);
  root->left->left = new Node(2);
  root->right->left = new Node(5);
  root->right->right = new Node(7);

  const auto levels = breadth_first_search(root);
  assert(levels.size() == 3);
  std::vector<int> level1_expected = {4};
  assert(levels[0] == level1_expected);
  std::vector<int> level2_expected = {3, 6};
  assert(levels[1] == level2_expected);
  std::vector<int> level3_expected = {2, 5, 7};
  assert(levels[2] == level3_expected);

  return 0;
}
