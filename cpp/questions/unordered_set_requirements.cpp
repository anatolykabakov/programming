#include <iostream>
#include <unordered_set>

struct Foo {
  int a;
};

bool operator==(const Foo& x, const Foo& y) { return x.a == y.a; }

namespace std {
template <>
struct hash<Foo> {
  typedef Foo argument_type;
  typedef size_t result_type;

  size_t operator()(const Foo& x) const { return x.a; }
};
}  // namespace std

int main()
{
  std::unordered_set<Foo> test;
  test.insert(Foo{42});

  return 0;
}
