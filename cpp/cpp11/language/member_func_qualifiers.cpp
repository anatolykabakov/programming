#include <iostream>
struct Bar {
    // ...
};

struct Foo {
    Bar getBar() & {
        std::cout << "Bar getBar() &" << std::endl;
        return bar;
    }
    Bar getBar() const & {
        std::cout << "Bar getBar() const&" << std::endl;
        return bar;
    }
    Bar getBar() && {
        std::cout << "Bar getBar() &&" << std::endl;
        return std::move(bar);
    }

private:
    Bar bar;
};

int main() {
    Foo foo{};
    Bar bar = foo.getBar();  // calls `Bar getBar() &`

    const Foo foo2{};
    Bar bar2 = foo2.getBar();  // calls `Bar Foo::getBar() const&`

    Foo{}.getBar();           // calls `Bar Foo::getBar() &&`
    std::move(foo).getBar();  // calls `Bar Foo::getBar() &&`

    std::move(foo2).getBar();  // calls `Bar Foo::getBar() const&&`
    return 0;
}
