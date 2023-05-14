// ref Exception Safety: Concepts and Techniques Bjarne Stroustrup
#include <iostream>
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
 * (1) allocate() throws an exception indicating that no memory is available;
 * (2) the allocator’s copy constructor throws an exception;
 * (3) the copy constructor for the element type Tthrows an exception because it can’t
 * copy val
 */
template <class T, class A>  // warning: naive implementation
vector<T, A>::vector(std::size_t n, const T &val, const A &a)
    : alloc(a)  // copy the allocator
{
    v = alloc.allocate(n);  // get memory for elements
    space = last = v + n;
    for (T *p = v; p != last; ++p)
        alloc.construct(p, val);  // construct copy of val in *p
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
    /**
     * When allocate() fails, the throwwill exit before any resources are acquired, so all
     * is well.
     *
     * When T’s copy constructor fails, we have acquired some memory that must be
     * freed to avoid memory leaks.
     *
     * A more difficult problem is that the copy constructor for T might throw an
     * exception after correctly constructing a few elements but before constructing them
     * all
     */

    vector<Foo> v(1, Foo{1});
    return 0;
}
