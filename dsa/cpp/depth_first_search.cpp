#include <iostream>
#include <stack>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int value) : value{value}, left{nullptr}, right{nullptr} {};
};

void dfs_preorder(Node* root)
{
  if (root == nullptr) {
    return;
  }
  std::cout << root->value << " ";
  dfs_preorder(root->left);
  dfs_preorder(root->right);
}

void dfs_inorder(Node* root)
{
  if (root == nullptr) {
    return;
  }
  dfs_inorder(root->left);
  std::cout << root->value << " ";
  dfs_inorder(root->right);
}

void dfs_postorder(Node* root)
{
  if (root == nullptr) {
    return;
  }
  dfs_postorder(root->left);
  dfs_postorder(root->right);
  std::cout << root->value << " ";
}

void dfs_inorder_iterative(Node* root)
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
      std::cout << cur->value << " ";
      cur = cur->right;
    }
  }
}

void dfs_preorder_iterative(Node* root)
{
  std::stack<Node*> s;
  Node* cur = root;
  while (cur || !s.empty()) {
    if (cur) {
      std::cout << cur->value << " ";
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
  root->left->left = new Node(2);
  root->right = new Node(6);
  root->right->left = new Node(5);
  root->right->right = new Node(7);

  dfs_inorder(root);  // 2 3 4 5 6 7
  std::cout << std::endl;
  dfs_preorder(root);  // 4 3 2 6 5 7
  std::cout << std::endl;
  dfs_postorder(root);  // 2 3 5 7 6 4
  std::cout << std::endl;
  dfs_inorder_iterative(root);  // 2 3 4 5 6 7
  std::cout << std::endl;
  dfs_preorder_iterative(root);  // 4 3 2 6 5 7
  std::cout << std::endl;
  return 0;
}
