#include <iostream>
#include <utility>
/**
 * TODO:
 * 1. custom deleter
 * 2. SFINAE for deleter
 * 3. make_unique (variadic templates)
 */
template <typename T>
struct myunique_ptr {
    myunique_ptr() : ptr_(nullptr) { std::cout << "myunique_ptr()" << std::endl; }
    myunique_ptr(T *ptr) : ptr_(ptr) { std::cout << "myunique_ptr(T *ptr)" << std::endl; }
    ~myunique_ptr() {
        std::cout << "~myunique_ptr()" << std::endl;
        delete ptr_;
    }
    T &operator*() const { return *ptr_; }
    T *operator->() const { return ptr_; }
    T *get() const { return ptr_; }
    T *release() {
        T *tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }
    void reset(T *p) {
        delete ptr_;
        ptr_ = p;
    }
    myunique_ptr(const myunique_ptr &) = delete;
    myunique_ptr &operator=(const myunique_ptr &) = delete;
    myunique_ptr(myunique_ptr &&other) : ptr_(other.release()) {
        std::cout << "myunique_ptr(myunique_ptr&& other):" << std::endl;
    }
    myunique_ptr &operator=(myunique_ptr &&other) {
        std::cout << "myunique_ptr& operator=(myunique_ptr&&)" << std::endl;
        reset(other.release());
        return *this;
    }

private:
    T *ptr_;
};

struct A {
    A() { std::cout << "A()" << std::endl; }
    A(int in) : data(in) { std::cout << "A(int)" << std::endl; }
    ~A() { std::cout << "~A()" << std::endl; }
    int data;
};

template <typename T, typename... Args>
myunique_ptr<T> make_myunique(Args &&... args) {
    return myunique_ptr<T>(new T(std::forward<Args>(args)...));
}

int main() {
    myunique_ptr<A> ptr(new A());
    myunique_ptr<A> ptrB = std::move(ptr);

    myunique_ptr<A> ptr2(make_myunique<A>(1));
    return 0;
}
