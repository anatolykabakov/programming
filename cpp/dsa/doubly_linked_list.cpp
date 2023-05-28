#include <iostream>

class DoublyLinkedList {
public:
  struct Node {
    Node* next;
    Node* prev;
    int value;
    Node(int x) : value{x}, next{nullptr}, prev{nullptr} {}
  };

  DoublyLinkedList() : size_(0)
  {
    head_ = new Node(-1);  // dummy node for easer insert and remove
    tail_ = new Node(-1);
    head_->next = tail_;
    tail_->prev = head_;
  }

  ~DoublyLinkedList()
  {
    Node* cur = head_;
    while (cur != nullptr) {
      Node* tmp = cur->next;
      delete cur;
      cur = tmp;
    }
  }
  // Time O(1)
  void insert_front(int value)
  {
    Node* node = new Node(value);
    node->prev = head_;
    node->next = head_->next;

    head_->next->prev = node;
    head_->next = node;

    size_ = +1;
  }
  // Time O(1)
  void insert_end(int value)
  {
    Node* node = new Node(value);
    node->next = tail_;
    node->prev = tail_->prev;

    tail_->prev->next = node;
    tail_->prev = node;

    size_ = +1;
  }

  void remove_front()
  {
    head_->next->next->prev = head_;
    head_->next = head_->next->next;
  }

  void remove_end()
  {
    tail_->prev->prev->next = tail_;
    tail_->prev = tail_->prev->prev;
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
  void insert(int index, int value)
  {
    int count = 0;
    Node* curr = head_;
    while (count <= index && curr != nullptr) {
      ++count;
      curr = curr->next;
    }

    size_ += 1;
    if (curr == tail_->prev) {
      insert_end(value);
      return;
    }
    if (curr == head_->next) {
      insert_front(value);
      return;
    }

    Node* node = new Node(value);
    node->next = curr;
    node->prev = curr->prev;

    curr->prev->next = node;
    curr->prev = node;
  }

  // Time O(n)
  void remove(int index)
  {
    int count = 0;
    Node* curr = head_;
    while (count <= index && curr != nullptr) {
      ++count;
      curr = curr->next;
    }

    size_ -= 1;
    if (curr == tail_->prev) {
      remove_end();
      return;
    }
    if (curr == head_->next) {
      remove_front();
      return;
    }
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;
  }

  int size() const { return size_; }

  const Node* head() const { return head_->next; };
  const Node* tail() const { return tail_->prev; };

private:
  Node* head_;
  Node* tail_;
  int size_;
};

void print(const DoublyLinkedList& list)
{
  const DoublyLinkedList::Node* node = list.head();
  while (node != nullptr) {
    std::cout << node->value << " -> ";
    node = node->next;
  }
  std::cout << "null" << std::endl;
}

int main()
{
  DoublyLinkedList ll;
  ll.insert_front(10);
  ll.insert_front(20);
  ll.insert_front(30);
  print(ll);
  ll.remove_front();
  print(ll);
  ll.remove_end();
  print(ll);
  ll.insert_end(0);
  ll.insert_end(1);
  ll.insert_end(2);
  ll.insert_end(3);
  print(ll);
  ll.remove(0);
  print(ll);
  ll.remove(3);
  print(ll);
  ll.remove(1);
  print(ll);
  ll.remove(1);
  print(ll);
  std::cout << ll.get(0) << std::endl;
  ll.insert(0, 11);
  print(ll);
  ll.insert(0, 9);
  print(ll);
  ll.insert(1, 10);
  print(ll);

  return 0;
}
