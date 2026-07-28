#include <iostream>

template <typename T>
class MyUniquePtr {
public:
  explicit MyUniquePtr(T* data) : data_(data) {}

  MyUniquePtr& operator=(const MyUniquePtr& other) = delete;
  MyUniquePtr(const MyUniquePtr& other) = delete;

  MyUniquePtr& operator=(MyUniquePtr&& other) noexcept
  {
    delete data_;
    data_ = other.data_;
    other.data_ = nullptr;
    return *this;
  }

  MyUniquePtr(MyUniquePtr&& other) noexcept : data_(other.data_) { other.data_ = nullptr; }

  ~MyUniquePtr()
  {
    if (data_) {
      delete data_;
    }
  }

private:
  T* data_;
};

struct Foo {
  Foo() { std::cout << "Foo ctor" << std::endl; }

  ~Foo() { std::cout << "Foo dtor" << std::endl; }
};

int main() { MyUniquePtr p(new Foo()); }
