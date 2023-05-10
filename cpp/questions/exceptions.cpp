#include <exception>
#include <iostream>

struct X {
    X() { std::cout << "X()" << std::endl; }
    ~X() { std::cout << "~X()" << std::endl; }
};

class Failure {
public:
    Failure() : m_x() {
        if (!--m_counter) throw std::logic_error("oops");
    }

    ~Failure() { ++m_counter; }

    X m_x;
    static int m_counter;
};

int Failure::m_counter = 5;

class Test {
public:
    Test() { std::cout << "Test()" << std::endl; }

    ~Test() { std::cout << "~Test()" << std::endl; }
};

class Test1 {
public:
    Test1() {
        std::cout << "Test1()" << std::endl;
        throw std::logic_error("oops");
    }

    ~Test1() { std::cout << "~Test1()" << std::endl; }
};

class Test2 {
public:
    Test2() { std::cout << "Test2()" << std::endl; }

    ~Test2() {
        std::cout << "~Test2()" << std::endl;
        // throw std::logic_error("oops"); error std::terminate, because dtoc
        // noexcept(true)
    }
};

int main() {
    try {
        Failure fails[10];
    } catch (const std::exception &x) {
        std::cout << x.what() << ": " << Failure::m_counter << std::endl;
    }

    try {
        Test t;
        throw std::logic_error("oops");
    } catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }

    try {
        Test t;    // dtoc called
        Test1 t1;  // dtoc not called
                   /** Destructors are only called for the completely constructed objects.
                    * When the constructor of an object throws an exception,
                    * the destructor for that object is not called. */
    } catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }

    try {
        Test2 t2;  // error, because dtoc noexcept(true)
    } catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}
