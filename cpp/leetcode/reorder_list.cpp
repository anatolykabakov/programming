

#include <iostream>
#include <vector>

struct ListNode {
  int val;
  ListNode* next;
  ListNode() : val(0), next(nullptr) {}
  ListNode(int x) : val(x), next(nullptr) {}
  ListNode(int x, ListNode* next) : val(x), next(next) {}
};

class Solution {
public:
  void reorderList(ListNode* head)
  {
    if (head->next == NULL) {
      return;
    }

    ListNode* prev = NULL;
    ListNode* slow = head;
    ListNode* fast = head;

    while (fast != NULL && fast->next != NULL) {
      prev = slow;
      slow = slow->next;
      fast = fast->next->next;
    }

    prev->next = NULL;

    ListNode* l1 = head;
    ListNode* l2 = reverse(slow);

    merge(l1, l2);
  }

private:
  ListNode* reverse(ListNode* head)
  {
    ListNode* prev{nullptr};
    ListNode* curr = head;
    ListNode* next = head->next;

    while (curr) {
      next = curr->next;
      curr->next = prev;
      prev = curr;
      curr = next;
    }

    return prev;
  }
  void merge(ListNode* l1, ListNode* l2)
  {
    while (l1) {
      ListNode* p1 = l1->next;
      ListNode* p2 = l2->next;

      l1->next = l2;
      if (p1 == nullptr) {
        break;
      }
      l2->next = p1;

      l1 = p1;
      l2 = p2;
    }
  }
};

int main()
{
  ListNode* n1 = new ListNode(1);
  ListNode* n2 = new ListNode(2);
  ListNode* n3 = new ListNode(3);
  ListNode* n4 = new ListNode(4);
  ListNode* n5 = new ListNode(5);
  n1->next = n2;
  n2->next = n3;
  n3->next = n4;
  n4->next = n5;

  Solution solution;
  solution.reorderList(n1);

  while (n1) {
    std::cout << n1->val << std::endl;
    n1 = n1->next;
  }

  return 0;
}
