#include <iostream>

template <typename Derived>
struct Base {
  int process()
  {
    self().reset();
    for (int i = 0; i < 3; ++i) {
      self().step(i);
    }
    return self().result();
  }

private:
  Derived& self() { return static_cast<Derived&>(*this); }
};

struct Derived : public Base<Derived> {
  void reset() { sum_ = 0; }

  void step(int x) { sum_ += x; }

  int result() { return sum_; }

private:
  int sum_{0};
};

int main()
{
  Derived d;
  std::cout << d.process() << std::endl;
}
