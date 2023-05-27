#include <iostream>
#include <vector>

template <typename T>
class mystack {
public:
  mystack() {}
  // Time: O(1)
  void push(const T& value) { arr_.push_back(value); }

  // Time: O(1)
  T pop()
  {
    T value = arr_[arr_.size() - 1];
    arr_.pop_back();
    return value;
  }

private:
  std::vector<T> arr_;
};

int main()
{
  mystack<int> s;
  s.push(1);
  s.push(2);
  s.push(3);
  std::cout << s.pop() << std::endl;
  std::cout << s.pop() << std::endl;
  std::cout << s.pop() << std::endl;
  return 0;
}
