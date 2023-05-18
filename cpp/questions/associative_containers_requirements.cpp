#include <iostream>
#include <set>

struct Foo {
  int i;
};

bool operator<(const Foo& a, const Foo& b) { return a.i < b.i; }

int main()
{
  std::set<Foo> s;
  s.insert(Foo{1});
  s.insert(Foo{2});
  s.insert(Foo{3});

  return 0;
}
