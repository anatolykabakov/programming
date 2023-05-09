#include <cassert>
#include <iostream>
#include <memory>

struct A {
    A() { std::cout << "A()" << std::endl; }
    A(const A &) { std::cout << "A(const A&)" << std::endl; }
    A(A &&) { std::cout << "A(A&&)" << std::endl; }
    void bar() { std::cout << "bar" << std::endl; }
    A &operator=(A &&) { std::cout << "A& operator=(A&&)" << std::endl; }
    ~A() { std::cout << "~A()" << std::endl; }
};

void unique_pass_by_ref(const std::unique_ptr<A> &ptr) { ptr->bar(); }

void unique_pass_by_value(std::unique_ptr<A> ptr) { ptr->bar(); }

void shared_pass_by_ref(const std::shared_ptr<A> &ptr) {
    std::cout << ptr.use_count() << std::endl;
}

void shared_pass_by_value(std::shared_ptr<A> ptr) {
    std::cout << ptr.use_count() << std::endl;
}

int main() {
    std::unique_ptr<A> p1{new A{}};  // `p1` owns `A`
    if (p1) {
        p1->bar();
    }

    {
        std::unique_ptr<A> p2{std::move(p1)};  // Now `p2` owns `A`

        p1 = std::move(p2);  // Ownership returns to `p1` -- `p2` gets destroyed
    }

    if (p1) {
        p1->bar();
    }

    unique_pass_by_ref(p1);
    assert(p1 != nullptr);

    unique_pass_by_value(std::move(p1));
    assert(p1 == nullptr);

    const auto s1 = std::make_shared<A>();
    shared_pass_by_ref(s1);
    shared_pass_by_value(s1);
}
