#include <iostream>

struct Node {
  int value = 0;
  Node* next{nullptr};
};

class MyQueue {
public:
  MyQueue() : head_{nullptr}, tail_{nullptr} {}

  void enqueue(int value)
  {
    Node* node = new Node{value, nullptr};
    if (tail_ == nullptr) {
      head_ = tail_ = node;
      return;
    }

    tail_->next = node;
    tail_ = tail_->next;
  }

  int dequeue()
  {
    if (head_ == nullptr) {
      return -1;
    }
    int value = head_->value;
    head_ = head_->next;
    if (head_ == nullptr) {
      tail_ = nullptr;
    }
    return value;
  }

private:
  Node* head_;
  Node* tail_;
};

int main()
{
  MyQueue q;
  q.enqueue(1);
  q.enqueue(2);
  q.enqueue(3);
  std::cout << q.dequeue() << std::endl;
  std::cout << q.dequeue() << std::endl;
  std::cout << q.dequeue() << std::endl;
  return 0;
}
