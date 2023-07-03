#include <iostream>
#include <stack>
#include <vector>
#include <cassert>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int value) : value{value}, left{nullptr}, right{nullptr} {};
};

/**
 * @brief The function returns the node values of binary tree preorder traversal.
 * The recursion algorithm:
 * 1. Recursion base case. Node is nullptr
 * 2. The root node value added to nums.
 * 3. Traversal the left subtree.
 * 4. Traversal the right subtree.
 *
 * Time O(n) Space O(n)
 *
 * @param[in] root The root of binary tree.
 * @param[out] nums The values of binary tree preorder traversal.
 */
void dfs_preorder(Node* root, std::vector<int>& nums)
{
  if (root == nullptr) {
    return;
  }
  nums.push_back(root->value);
  dfs_preorder(root->left, nums);
  dfs_preorder(root->right, nums);
}

/**
 * @brief The function returns the node values of binary tree in inorder traversal.
 * The recursion algorithm:
 * 1. Recursion base case. Node is nullptr
 * 2. Traversal the left subtree.
 * 3. The root node value added to nums.
 * 4. Traversal the right subtree.
 *
 * Time O(n) Space O(n)
 *
 * @param[in] root The root of binary tree.
 * @param[out] nums The values of binary tree inorder traversal.
 */
void dfs_inorder(Node* root, std::vector<int>& nums)
{
  if (root == nullptr) {
    return;
  }
  dfs_inorder(root->left, nums);
  nums.push_back(root->value);
  dfs_inorder(root->right, nums);
}

/**
 * @brief The function returns the node values of binary tree in postorder traversal.
 * The recursion algorithm:
 * 1. Recursion base case. Node is nullptr
 * 2. Traversal the left subtree.
 * 3. Traversal the right subtree.
 * 4. The root node value added to nums.
 *
 * Time O(n) Space O(n)
 *
 * @param[in] root The root of binary tree.
 * @param[out] nums The values of binary tree in postorder traversal.
 */
void dfs_postorder(Node* root, std::vector<int>& nums)
{
  if (root == nullptr) {
    return;
  }
  dfs_postorder(root->left, nums);
  dfs_postorder(root->right, nums);
  nums.push_back(root->value);
}

/**
 * @brief The function returns the node values of binary tree inorder traversal.
 * The iterative algorithm.
 *
 * Time O(n) Space O(n)
 *
 * @param[in] root The root of binary tree.
 * @param[out] nums The values of binary tree in inorder traversal.
 */
void dfs_inorder_iterative(Node* root, std::vector<int>& nums)
{
  std::stack<Node*> s;
  Node* cur = root;
  while (cur || !s.empty()) {
    if (cur) {
      s.push(cur);
      cur = cur->left;
    } else {
      cur = s.top();
      s.pop();
      nums.push_back(cur->value);
      cur = cur->right;
    }
  }
}

/**
 * @brief The function returns the node values of binary tree inorder traversal.
 * The iterative algorithm.
 *
 * Time O(n) Space O(n)
 *
 * @param[in] root The root of binary tree.
 * @param[out] nums The values of binary tree in inorder traversal.
 */
void dfs_preorder_iterative(Node* root, std::vector<int>& nums)
{
  std::stack<Node*> s;
  Node* cur = root;
  while (cur || !s.empty()) {
    if (cur) {
      nums.push_back(cur->value);
      if (cur->right) {
        s.push(cur->right);
      }
      cur = cur->left;
    } else {
      cur = s.top();
      s.pop();
    }
  }
}

int main()
{
  Node* root = new Node(4);
  root->left = new Node(3);
  root->right = new Node(6);
  root->left->left = new Node(2);
  root->right->left = new Node(5);
  root->right->right = new Node(7);

  std::vector<int> inorder_expected = {2, 3, 4, 5, 6, 7};
  std::vector<int> inorder_result;
  dfs_inorder(root, inorder_result);
  assert(inorder_expected == inorder_result);

  inorder_result.clear();
  dfs_inorder_iterative(root, inorder_result);
  assert(inorder_expected == inorder_result);

  std::vector<int> preorder_expected = {4, 3, 2, 6, 5, 7};
  std::vector<int> preorder_result;
  dfs_preorder(root, preorder_result);
  assert(preorder_expected == preorder_result);

  preorder_result.clear();
  dfs_preorder_iterative(root, preorder_result);
  assert(preorder_expected == preorder_result);

  std::vector<int> postorder_expected = {2, 3, 5, 7, 6, 4};
  std::vector<int> postorder_result;
  dfs_postorder(root, postorder_result);
  assert(postorder_expected == postorder_result);
  return 0;
}
