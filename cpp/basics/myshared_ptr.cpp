#include <iostream>
#include <atomic>

struct ControlBlock {
  ControlBlock() { std::cout << "ControlBlock" << std::endl; }
  ~ControlBlock() { std::cout << "~ControlBlock" << std::endl; }
  std::atomic<int> counter{0};
};

template <typename T>
class MySharedPtr {
public:
  MySharedPtr(T* ptr) : ptr_(ptr)
  {
    std::cout << "MySharedPtr(T *ptr)" << std::endl;
    cb_ = new ControlBlock();
    ++cb_->counter;
  }

  MySharedPtr(MySharedPtr& other) : ptr_(other.ptr_), cb_(other.cb_)
  {
    std::cout << "MySharedPtr(MySharedPtr &other)" << std::endl;
    if (cb_) {
      ++cb_->counter;
    }
  }

  MySharedPtr& operator=(MySharedPtr& other)
  {
    std::cout << "MySharedPtr& operator=(MySharedPtr &other)" << std::endl;
    MySharedPtr(other).swap(*this);
    return *this;
  }

  MySharedPtr(MySharedPtr&& other) noexcept ptr_(other.ptr_), cb_(other.cb_)
  {
    ptr_ = nullptr;
    cb_ = nullptr;
  }

  MySharedPtr& operator=(MySharedPtr&& other) noexcept
  {
    std::cout << "MySharedPtr& operator=(MySharedPtr &other)" << std::endl;
    MySharedPtr(std::move(other)).swap(*this);
    return *this;
  }

  ~MySharedPtr()
  {
    std::cout << "~MySharedPtr()" << std::endl;
    release_();
  }

  void swap(MySharedPtr& other)
  {
    std::swap(ptr_, other.ptr_);
    std::swap(cb_, other.cb_);
  }

  T* operator->() { return ptr_; }

  int use_count() { return cb_ ? cb_->counter.load() : 0; }

private:
  T* ptr_;
  ControlBlock* cb_;

  void release_()
  {
    if (cb_->counter.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete ptr_;
      delete cb_;
    }
  }
};

struct Foo {
  Foo() { std::cout << "Foo::ctor" << std::endl; }

  ~Foo() { std::cout << "Foo::dtor" << std::endl; }
};

int main()
{
  MySharedPtr s1(new Foo());
  std::cout << s1.use_count() << std::endl;
  MySharedPtr s2(s1);
  std::cout << s2.use_count() << std::endl;
  s2 = s1;
  std::cout << s1.use_count() << " " << s2.use_count() << std::endl;
}
