/**
  The basic rules are:
  (1) When updating an object, don’t destroy its old representation before a new
 representation is completely constructed and can replace the old one without risk of
 exceptions. For example, see the implementations of safe_assign() and vector:
 :push_back() in Sect. 3. (1a) If you must override an old representation in the process
 of creating the new, be sure to leave a valid object behind if an exception is thrown.
 For example, see the ‘‘optimized’’ implementation of vector: :operator=(). (2) Before
 throwing an exception, release every resource acquired that is not owned by some (other)
 object. (2a) The ‘‘resource acquisition is initialization’’ technique and the language
  rule that partially constructed objects are destroyed to the extent that they
  were constructed can be most helpful here.
  (2b) The uninitialized_copy() algorithm and its cousins provide automatic
  release of resources in case of failure to complete construction of a set of
  objects.
  (3) Before throwing an exception, make sure that every operand is in a valid state.
  That is, leave each object in a state that allows it to be accessed and destroyed
  without causing undefined behavior or an exception to be thrown from a
  destructor. For example, see vector’s assignment in Sect. 3.2.
  (3a) Note that constructors are special in that when an exception is thrown from
  a constructor, no object is left behind to be destroyed later. This implies
  that we don’t have to establish an invariant and that we must be sure to
  release all resources acquired during a failed construction before throwing
  an exception.
  (3b) Note that destructors are special in that an exception thrown from a
  destructor almost certainly leads to violation of invariants and/or calls to
  terminate()
 *
 */

#include <iostream>
#include <memory>

template <class T, class A = std::allocator<T> >
struct vector_base {  // RAII object
    T *v;             // start of allocation
    T *space;         // end of elements, start of space allocated for possible expansion
    T *last;          // end of allocated space
    A alloc;          // allocator
    vector_base(std::size_t n, const A &a)
        : alloc(a), v(alloc.allocate(n)), last(v + n), space(v + n) {
        std::cout << "vector_base ctor" << std::endl;
    };

    ~vector_base() {
        alloc.deallocate(v, last - v);
        std::cout << "vector_base dtor" << std::endl;
    }  // destructor
};

template <class T>
void swap(vector_base<T> &a, vector_base<T> &b) {
    std::swap(a.a, b.a);
    std::swap(a.v, b.v);
    std::swap(a.space, b.space);
    std::swap(a.last, b.last);
}

template <class T, class A = std::allocator<T> >
class vector : private vector_base<T, A> {
    void destroy_elements() {
        for (T *p = this->v; p != this->space; ++p) p->~T();
    }

public:
    vector(std::size_t n, const T &val = T(), const A &a = A());
    vector(const vector &a);                                // copy constructor
    vector &operator=(const vector &a);                     // copy assignment
    ~vector() { std::cout << "vector dtor" << std::endl; }  // destructor
    std::size_t size() const { return this->space - this->v; }
    std::size_t capacity() const { return this->last - this->v; }
    void push_back(const T &);
};

template <class T, class A>  // warning: naive implementation
vector<T, A>::vector(std::size_t n, const T &val, const A &a) : vector_base<T, A>(n, a) {
    std::cout << "vector ctor" << std::endl;
    std::uninitialized_fill(this->v, this->v + n, val);  // copy elements
};

template <class T, class A>
vector<T, A>::vector(const vector<T, A> &a) : vector_base<T, A>(a.size(), a.alloc) {
    std::cout << "vector copy ctor" << std::endl;
    std::uninitialized_copy(a.begin(), a.end(), this->v);
}

template <class T, class A>  // offers the strong guarantee
vector<T, A> &vector<T, A>::operator=(const vector &a) {
    // vector_base<T,A> b(this->alloc, a.size()) ; // get memory
    // std::uninitialized_copy(a.begin() ,a.end(), b.v) ; // copy elements
    // destroy_elements() ; // destroy old elements
    // this->alloc.deallocate(this->v, this->last-this->v) ; // free old memory
    // vector_base<T, A>::operator=(b) ; // install new representation
    // b.v= 0; // prevent deallocation

    vector temp(a);
    std::swap(*this, temp);
    return *this;
}

template <class T, class A>
void vector<T, A>::push_back(const T &x) {
    if (this->space == this->last) {  // no more free space; relocate:
        vector_base<T, A> b(size() ? 2 * size() : 2,
                            this->alloc);  // double the allocation
        std::uninitialized_copy(this->v, this->space, b.v);
        new (b.space) T(x);  // place a copy of x in *b.space
        ++b.space;
        destroy_elements();
        std::swap<vector_base<T, A> >(b, *this);  // swap representations
        return;
    }
    new (this->space) T(x);  // place a copy of x in *space
    ++this->space;
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
        v.push_back(Foo{2});
    } catch (...) {
        std::cout << "catch" << std::endl;
    }
    return 0;
}
