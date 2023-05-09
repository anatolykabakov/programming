#include <iostream>

template <typename T>
struct myshared_ptr {
    myshared_ptr() : ptr_(nullptr), ref_counter_{0} {
        std::cout << "myshared_ptr()" << std::endl;
    }
    explicit myshared_ptr(T *ptr) : ptr_(ptr), ref_counter_{1} {
        std::cout << "myshared_ptr(T *ptr)" << std::endl;
    }
    myshared_ptr(const myshared_ptr &other) {
        ptr_ = other.ptr_;
        ref_counter_ = other.ref_counter_;
        ++ref_counter_;
        std::cout << "myshared_ptr(const myshared_ptr& other)" << std::endl;
    }
    myshared_ptr &operator=(const myshared_ptr &other) {
        ptr_ = other.ptr_;
        ref_counter_ = other.ref_counter_;
        ++ref_counter_;
        std::cout << "myshared_ptr& operator=(const myshared_ptr& other)" << std::endl;
        return *this;
    }
    ~myshared_ptr() {
        std::cout << "~myshared_ptr()" << std::endl;
        --ref_counter_;
        if (ref_counter_ == 0) {
            delete ptr_;
        }
    }
    std::size_t use_count() { return ref_counter_; }

private:
    T *ptr_;
    std::size_t ref_counter_;
};

struct A {
    A() { std::cout << "A()" << std::endl; }
    ~A() { std::cout << "~A()" << std::endl; }
    int data;
};

int main() {
    myshared_ptr<A> s(new A());
    std::cout << s.use_count() << std::endl;
    myshared_ptr<A> c = s;
    std::cout << s.use_count() << std::endl;
    std::cout << c.use_count() << std::endl;
    return 0;
}
