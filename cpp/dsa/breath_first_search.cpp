#include <iostream>
#include <queue>

struct Node {
  Node* left;
  Node* right;
  int value;
  Node(int value) : value{value}, left{nullptr}, right{nullptr} {};
};

void breadth_first_search(Node* root)
{
  std::queue<Node*> q;
  if (root) {
    q.push(root);
  }
  int level = 0;
  while (!q.empty()) {
    Node* cur = q.front();
    q.pop();
    std::cout << cur->value << " ";
    if (cur->left) {
      q.push(cur->left);
    }
    if (cur->right) {
      q.push(cur->right);
    }
  }
}

void breadth_first_search_with_level(Node* root)
{
  std::queue<Node*> q;
  if (root) {
    q.push(root);
  }
  int level = 0;
  while (!q.empty()) {
    std::cout << "level = " << level << std::endl;
    int size = q.size();
    for (uint i = 0; i < size; ++i) {
      Node* cur = q.front();
      q.pop();
      std::cout << cur->value << " ";
      if (cur->left) {
        q.push(cur->left);
      }
      if (cur->right) {
        q.push(cur->right);
      }
    }
    ++level;
    std::cout << std::endl;
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
  breadth_first_search(root);
  std::cout << std::endl;
  breadth_first_search_with_level(root);
  return 0;
}
