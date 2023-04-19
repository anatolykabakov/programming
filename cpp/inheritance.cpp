#include <iostream>

struct base {
  base();
  void (*foo)(base*);
  void (*bar)(base*, int);
};

void foo_base(base* self) { std::cout << __func__ << std::endl; }

void bar_base(base* self, int i) { std::cout << __func__ << std::endl; }

base::base() : foo(foo_base), bar(bar_base) {}

struct derived : base {
  derived();
};

void foo_derived(base* self) { std::cout << __func__ << std::endl; }

void bar_derived(base* self, int i) { std::cout << __func__ << std::endl; }

derived::derived() : base() {
  foo = foo_derived;
  bar = bar_derived;
}

int main() {
  base b;
  b.foo(&b);
  // b.bar(1);

  derived d;
  d.foo(&d);
}
