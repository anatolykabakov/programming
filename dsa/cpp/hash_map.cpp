#include <cassert>
#include <iostream>

class Pair {
public:
  Pair* next;
  Pair(int k, int v)
  {
    key_ = k;
    value_ = v;
    next = nullptr;
  }
  int key() const { return key_; }
  int value() const { return value_; }
  void change_value(int value) { value_ = value; }

private:
  int key_;
  int value_;
};
/**
 * @brief HashMap is data structure which is used to store key value pairs.
 * @ref
 * https://www.tutorialspoint.com/cplusplus-program-to-implement-hash-tables-chaining-with-list-heads
 *
 */
class HashMap {
public:
  HashMap(int size) : size_(size)
  {
    array_ = new Pair*[size_];
    for (int i = 0; i < size_; ++i) {
      array_[i] = nullptr;
    }
  }

  void insert(int k, int v)
  {
    int index = hash_function_(k);
    if (array_[index] == nullptr) {
      array_[index] = new Pair(k, v);
    } else {
      Pair* el = array_[index];
      while (el->next) {
        el = el->next;
      }
      if (el->key() == k) {
        el->change_value(v);
      } else {
        el->next = new Pair(k, v);
      }
    }
  }

  Pair* get(int k)
  {
    int index = hash_function_(k);

    Pair* el = array_[index];
    while (el && el->key() != k) {
      el = el->next;
    }
    return el;  // return nullptr or Pair*
  }

  void remove(int k)
  {
    int index = hash_function_(k);
    if (array_[index] != nullptr) {
      Pair* el = array_[index];
      Pair* prev = nullptr;
      while (el->next && el->key() != k) {
        prev = el;
        el = el->next;
      }

      if (el->key() == k) {
        Pair* next = el->next;
        delete el;
        if (prev == nullptr) {
          array_[index] = next;
        } else {
          prev->next = next;
        }
      }
    }
  }

  ~HashMap()
  {
    for (int i = 0; i < size_; ++i) {
      if (array_[i] != nullptr) {
        delete array_[i];
      }
    }
    delete[] array_;
  }

private:
  Pair** array_;
  int size_;

private:
  int hash_function_(int k) { return k % size_; }
};

int main()
{
  HashMap hash_table(1);
  hash_table.insert(1, 2);
  Pair* p = hash_table.get(1);
  std::cout << "key " << p->key() << " value " << p->value() << std::endl;
  hash_table.remove(1);
  p = hash_table.get(1);
  assert(p == nullptr);

  return 0;
}
