#include <iostream>
#include <atomic>

template <typename T>
class MyUniquePtr {
public:
  explicit MyUniquePtr(T* ptr) : ptr_(ptr) {}
  MyUniquePtr(const MyUniquePtr&) = delete;
  MyUniquePtr& operator=(const MyUniquePtr&) = delete;

  MyUniquePtr(MyUniquePtr&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
  MyUniquePtr& operator=(MyUniquePtr&& o) noexcept
  {
    MyUniquePtr(std::move(o)).swap(*this);
    return *this;
  }
  T* get() const { return ptr_; }
  T* operator->() const { return ptr_; }
  T* release()
  {
    T* tmp = ptr_;
    ptr_ = nullptr;
    return tmp;
  }
  ~MyUniquePtr() { delete ptr_; }

private:
  T* ptr_;

  void swap(MyUniquePtr& o) noexcept { std::swap(o.ptr_, ptr_); }
};

template <typename T>
class MySharedPtr {
public:
  explicit MySharedPtr(T* ptr) : ptr_(ptr)
  {
    try {
      cb_ = new ControlBlock(1);
    } catch (...) {
      delete ptr_;
      throw;
    }
  }

  MySharedPtr(const MySharedPtr& o) : ptr_(o.ptr_), cb_(o.cb_) { ++cb_->count; }
  MySharedPtr& operator=(const MySharedPtr& o) noexcept
  {
    MySharedPtr(o).swap(*this);
    return *this;
  }
  MySharedPtr(MySharedPtr&& o) : ptr_(o.ptr_), cb_(o.cb_)
  {
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
  }
  MySharedPtr& operator=(MySharedPtr&& o) noexcept
  {
    MySharedPtr(std::move(o)).swap(*this);
    return *this;
  }

  ~MySharedPtr() { release(); }

private:
  T* ptr_;
  struct ControlBlock {
    std::atomic<int> count;
  };

  ControlBlock* cb_;

  void release()
  {
    if (!cb_)
      return;

    if (cb_->counter.fetch_sub(1, std::memory_order_acq_rel)) {
      delete ptr_;
      delete cb_;
    }
  }

  void swap(MySharedPtr& o)
  {
    std::swap(o.ptr_, ptr_);
    std::swap(o.cb_, cb_);
  }
};

int main() {}
