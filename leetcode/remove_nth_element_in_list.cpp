
/***
 * Given the head of a linked list, remove the nth node from the end of the list and return its head.
 * Input: head = [1,2,3,4,5], n = 2 Output: [1,2,3,5]
 * Input: head = [1], n = 1 Output: []
 */
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
  ListNode* removeNthFromEnd(ListNode* head, int n) {
    ListNode* n1 = reverse(head);
    ListNode* n2 = remove(n1, n);
    ListNode* n3 = reverse(n2);
    return n3;
  }

  ListNode* reverse(ListNode* head) {
    if (head == nullptr) {
      return nullptr;
    }
    ListNode* prev{nullptr};
    ListNode* curr{head};
    ListNode* next{head->next};

    while (curr) {
      next = curr->next;
      curr->next = prev;
      prev = curr;
      curr = next;
    }
    return prev;
  }

  ListNode* remove(ListNode* head, int n) {
    if (n == 1) {
      return head->next;
    }
    int count = 1;
    ListNode* prev{nullptr};
    ListNode* curr = head;

    while (curr) {
      if (count == n) {
        prev->next = curr->next;
        break;
      }
      prev = curr;
      curr = curr->next;
      count++;
    }
    return head;
  }
};

ListNode* testcase1() {
  ListNode* n1 = new ListNode(1);
  ListNode* n2 = new ListNode(2);
  ListNode* n3 = new ListNode(3);
  ListNode* n4 = new ListNode(4);
  ListNode* n5 = new ListNode(5);
  n1->next = n2;
  n2->next = n3;
  n3->next = n4;
  n4->next = n5;
  return n1;
}

ListNode* testcase2() {
  ListNode* n1 = new ListNode(1);
  return n1;
}

ListNode* testcase3() {
  ListNode* n1 = new ListNode(1);
  ListNode* n2 = new ListNode(2);
  n1->next = n2;
  return n1;
}

int main() {
  ListNode* testcase = testcase2();
  Solution solution;
  ListNode* ans = solution.removeNthFromEnd(testcase, 1);

  while (ans) {
    std::cout << ans->val << std::endl;
    ans = ans->next;
  }

  return 0;
}
