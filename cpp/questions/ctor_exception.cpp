#include <iostream>
#include <memory>

struct Resource {
    Resource() { std::cout << "ctor Resource" << std::endl; }
    ~Resource() { std::cout << "dtor Resource" << std::endl; }
};

class Bad {
public:
    Bad() {
        std::cout << "Bad()" << std::endl;
        std::cout << "open file" << std::endl;
        r = new Resource();
        throw std::logic_error("except");
    }
    ~Bad() {
        std::cout << "~Bad()" << std::endl;
        std::cout << "close file" << std::endl;
    }

private:
    Resource *r;
};

class Good {
public:
    Good() {
        std::cout << "Good()" << std::endl;
        std::cout << "open file" << std::endl;
        r = std::make_unique<Resource>();
        throw std::logic_error("except");
    }
    ~Good() {
        std::cout << "~Good()" << std::endl;
        std::cout << "close file" << std::endl;
    }

private:
    std::unique_ptr<Resource> r;
};

int main() {
    Bad *b;
    Good *g;
    try {
        b = new Bad();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    try {
        g = new Good();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    // delete b; ERROR
    // delete g; ERROR
    return 0;
}
