#include <iostream>

template <typename T>
struct myshared_ptr {
  myshared_ptr() : ptr_{nullptr}, ref_counter_{new std::size_t{0}} { std::cout << "myshared_ptr()" << std::endl; }
  explicit myshared_ptr(T* ptr) : ptr_{ptr}, ref_counter_{new std::size_t{1}}
  {
    std::cout << "myshared_ptr(T *ptr)" << std::endl;
  }
  myshared_ptr(const myshared_ptr& other)
  {
    ptr_ = other.ptr_;
    ref_counter_ = other.ref_counter_;
    if (other.ptr_ != nullptr) {
      ++(*ref_counter_);
    }
    std::cout << "myshared_ptr(const myshared_ptr& other)" << std::endl;
  }
  myshared_ptr& operator=(const myshared_ptr& other)
  {
    cleanup_();
    ptr_ = other.ptr_;
    ref_counter_ = other.ref_counter_;
    if (other.ptr_ != nullptr) {
      ++(*ref_counter_);
    }
    std::cout << "myshared_ptr& operator=(const myshared_ptr& other)" << std::endl;
    return *this;
  }
  myshared_ptr(myshared_ptr&& other)
  {
    std::swap(ptr_, other.ptr_);
    std::swap(ref_counter_, other.ref_counter_);
    other.ptr_ = nullptr;
    other.ref_counter_ = nullptr;
    std::cout << "myshared_ptr(const myshared_ptr&& other)" << std::endl;
  }
  myshared_ptr& operator=(myshared_ptr&& other)
  {
    cleanup_();
    std::swap(ptr_, other.ptr_);
    std::swap(ref_counter_, other.ref_counter_);
    other.ptr_ = nullptr;
    other.ref_counter_ = nullptr;
    std::cout << "myshared_ptr& operator=(myshared_ptr&& other)" << std::endl;
    return *this;
  }
  ~myshared_ptr()
  {
    std::cout << "~myshared_ptr()" << std::endl;
    if (ref_counter_)
      cleanup_();
  }
  std::size_t use_count() { return *ref_counter_; }

private:
  T* ptr_;
  std::size_t* ref_counter_;

private:
  void cleanup_()
  {
    --(*ref_counter_);
    if (ref_counter_ == 0) {
      if (ptr_ != nullptr) {
        delete ptr_;
      }
      delete ref_counter_;
    }
  }
};

template <typename T, typename... Args>
myshared_ptr<T> make_myshared(Args&&... args)
{
  return myshared_ptr<T>(new T(std::forward<T>(args)...));
}

struct A {
  A(int data) : data_{data} { std::cout << "A()" << std::endl; }
  ~A() { std::cout << "~A()" << std::endl; }
  int data_;
};

int main()
{
  myshared_ptr<A> p1(new A(1));
  myshared_ptr<A> p2(new A(2));
  std::cout << p1.use_count() << std::endl;
  std::cout << p2.use_count() << std::endl;
  p1 = p2;
  std::cout << p1.use_count() << std::endl;
  std::cout << p2.use_count() << std::endl;

  myshared_ptr<A> p3(p1);
  std::cout << p1.use_count() << std::endl;
  std::cout << p2.use_count() << std::endl;
  std::cout << p3.use_count() << std::endl;

  myshared_ptr<A> p4 = std::move(p1);
  // std::cout << p1.use_count() << std::endl; // ptr == nullptr
  std::cout << p2.use_count() << std::endl;
  std::cout << p3.use_count() << std::endl;
  std::cout << p4.use_count() << std::endl;

  return 0;
}
