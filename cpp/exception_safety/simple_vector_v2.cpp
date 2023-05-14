#include <iostream>
#include <iterator>
#include <memory>

template <class T, class A = std::allocator<T> >
class vector {
public:
    T *v;      // start of allocation
    T *space;  // end of elements, start of space allocated for possible expansion
    T *last;   // end of allocated space
    A alloc;   // allocator
    explicit vector(std::size_t n, const T &val = T(), const A & = A());
    vector(const vector &a);             // copy constructor
    vector &operator=(const vector &a);  // copy assignment
    ~vector() {}                         // destructor
    std::size_t size() const { return space - v; }
    std::size_t capacity() const { return last - v; }
    void push_back(const T &);
};
/***
 * we could keep track of which elements have been constructed
 * and destroy those (and only those) in case of an error
 */
template <class T, class A>  // warning: naive implementation
vector<T, A>::vector(std::size_t n, const T &val, const A &a)
    : alloc(a)  // copy the allocator
{
    v = alloc.allocate(n);  // get memory for elements
    T *p;
    try {
        T *end = v + n;
        for (p = v; p != end; ++p) alloc.construct(p, val);  // construct element
        last = space = p;
    } catch (...) {  // destroy constructed elements, free memory, and re-throw:
        for (T *q = v; q != p; ++q) alloc.destroy(q);
        alloc.deallocate(v, n);
        throw;
    }
}

struct Foo {
    explicit Foo(int i) : data{i} {
        // throw 1;
        std::cout << "Foo(int i)" << std::endl;
    }
    Foo(const Foo &) {
        throw 1;
        std::cout << "Foo(const Foo&)" << std::endl;
    }
    int data;
};

int main() {
    try {
        vector<Foo> v(1, Foo{1});
    } catch (...) {
        std::cout << "catch" << std::endl;
    }
    return 0;
}
