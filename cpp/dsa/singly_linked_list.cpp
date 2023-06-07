#include <iostream>

class SinglyLinkedList {
public:
  struct Node {
    Node* next;
    int value;
    Node(int x) : value{x}, next{nullptr} {}
  };

  SinglyLinkedList() : size_(0)
  {
    head_ = new Node(-1);  // dummy node for easer insert and remove
    tail_ = head_;
  }

  ~SinglyLinkedList()
  {
    Node* cur = head_;
    while (cur != nullptr) {
      Node* tmp = cur->next;
      delete cur;
      cur = tmp;
    }
  }

  // Time O(1)
  void insert_end(int value)
  {
    tail_->next = new Node(value);
    tail_ = tail_->next;
    size_ = +1;
  }

  // Time O(n)
  int get(int index)
  {
    int count = 0;
    Node* cur = head_->next;
    while (count < index && cur->next != nullptr) {
      ++count;
      cur = cur->next;
    }
    return cur->value;
  }

  // Time O(n)
  void remove(int index)
  {
    int count = 0;
    Node* prev = head_;
    while (count < index && prev != nullptr) {
      ++count;
      prev = prev->next;
    }

    if (prev && prev->next) {
      if (prev->next == tail_) {
        tail_ = prev;
      }
      prev->next = prev->next->next;
    }
    size_ -= 1;
  }

  int size() const { return size_; }

  const Node* root() const { return head_->next; };

private:
  Node* head_;
  Node* tail_;
  int size_;
};

void print(const SinglyLinkedList& list)
{
  const SinglyLinkedList::Node* node = list.root();
  while (node != nullptr) {
    std::cout << node->value << " -> ";
    node = node->next;
  }
  std::cout << "null" << std::endl;
}

int main()
{
  SinglyLinkedList ll;
  ll.insert_end(0);
  ll.insert_end(1);
  ll.insert_end(2);
  ll.insert_end(3);
  print(ll);
  ll.remove(3);
  print(ll);
  ll.remove(1);
  print(ll);
  std::cout << ll.get(0) << std::endl;
  std::cout << ll.get(1) << std::endl;

  return 0;
}
