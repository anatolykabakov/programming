

#include <vector>
#include <iostream>

struct ListNode {
  int val;
  ListNode* next;
  ListNode() : val(0), next(nullptr) {}
  ListNode(int x) : val(x), next(nullptr) {}
  ListNode(int x, ListNode* next) : val(x), next(next) {}
};

class Solution {
public:
  void reorderList(ListNode* head) {
    ListNode* slow = head;
    ListNode* fast = head->next;

    ListNode* prev{nullptr};
    while (fast && fast->next) {
      prev = slow;
      slow = slow->next;
      fast = fast->next->next;
    }
    // ListNode* tmp = slow->next;
    ListNode* l2 = reverse(slow->next);  // ok
    slow->next = nullptr;
    ListNode* l1 = head;
    // ListNode* l2 = slow;
    merge(l1, l2);
    while (l1) {
      std::cout << l1->val << std::endl;
      l1 = l1->next;
    }
    // std::cout << l2->val << std::endl;
  }

  ListNode* reverse(ListNode* root) {
    // input: 1 -> 2 -> 3 -> 4 -> null
    // output: 4 -> 3 -> 2 -> 1 -> null
    ListNode* prev{nullptr};
    while (root) {
      ListNode* tmp = root->next;  // 2
      root->next = prev;           // 1 -> 0
      prev = root;                 // 1
      root = tmp;                  // 2
    }

    return prev;
  }

  void merge(ListNode* l1, ListNode* l2) {
    // l1: 1 -> 2 -> 3
    // l2: 5 -> 4
    // out: 1 -> 5 -> 2 -> 4 -> 3

    ListNode* dummy = l1;

    while (dummy) {
      ListNode* tmp = dummy->next;  // val 2 next 3
      dummy->next = l2;             // 1 -> 5 -> 2
      dummy = tmp;                  // 2
      l2 = l2->next;
    }
  }
};

int main() {
  ListNode* root = new ListNode(1);
  ListNode* one = new ListNode(2);
  ListNode* two = new ListNode(3);
  ListNode* three = new ListNode(4);
  ListNode* four = new ListNode(5);
  root->next = one;
  one->next = two;
  two->next = three;
  three->next = four;

  Solution solution;
  solution.reorderList(root);

  // while (root)
  // {
  //   std::cout << root->val << std::endl;
  //   root = root->next;
  // }

  return 0;
}
