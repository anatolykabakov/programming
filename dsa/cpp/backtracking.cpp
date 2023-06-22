
#include <iostream>
#include <vector>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int value) : value{value}, left{nullptr}, right{nullptr} {};
};

// Time O(N)
bool is_path_exists(Node* root)
{
  // Base case
  if (root == nullptr || root->value == 0) {
    return false;
  }
  // Is node leaf?
  if (root->left == nullptr && root->right == nullptr) {
    return true;
  }
  if (is_path_exists(root->left)) {
    return true;
  }
  if (is_path_exists(root->right)) {
    return true;
  }
  return false;
}

// Time O(N) Space O(N)
bool find_path_without_zeros(Node* root, std::vector<Node*>& path)
{
  // Base case
  if (root == nullptr || root->value == 0) {
    return false;
  }
  path.push_back(root);
  // Is node leaf?
  if (root->left == nullptr && root->right == nullptr) {
    return true;
  }
  if (find_path_without_zeros(root->left, path)) {
    return true;
  }
  if (find_path_without_zeros(root->right, path)) {
    return true;
  }
  path.pop_back();
  return false;
}

int main()
{
  /***
   * Determine if a path exists from the root of the binary tree to the leaf node.
   * Path may not contain any nodes with zero values.
   */
  Node* root = new Node(4);
  root->left = new Node(0);
  root->right = new Node(1);
  root->left->left = new Node(2);
  root->right->left = new Node(5);
  root->right->right = new Node(0);
  std::cout << is_path_exists(root) << std::endl;
  root->right->left->value = 0;
  std::cout << is_path_exists(root) << std::endl;
  /***
   * Determine if a path exists from the root of the binary tree to the leaf node.
   * Path may not contain any nodes with zero values.
   * Return true and path if path already exists, false and an empty array otherwise.
   */
  std::vector<Node*> path;
  std::cout << find_path_without_zeros(root, path) << std::endl;
  for (const auto& node : path) {
    std::cout << node->value << std::endl;
  }
  root->right->left->value = 5;
  std::cout << find_path_without_zeros(root, path) << std::endl;
  for (const auto& node : path) {
    std::cout << node->value << std::endl;
  }
  return 0;
}
