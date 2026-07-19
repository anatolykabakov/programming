#include <iostream>
#include <vector>

class Foo {
public:
  Foo(const std::vector<int> nums) : nums_(std::move(nums)), size_(nums_.size()) { std::cout << "ctor" << std::endl; }

  int update() { return one(); }

  int size() const { return size_; }

  ~Foo() { std::cout << "dtor" << std::endl; }

private:
  std::vector<int> nums_;
  int size_{0};

  int one()
  {
    std::cout << "do one" << std::endl;
    return 42;
  }
};

class Builder {
public:
  Builder& add(int x)
  {
    data_.push_back(x);
    ++count;
    return *this;
  }

  void build()
  {
    for (const auto& val : data_) {
      std::cout << val << " ";
    }
    std::cout << std::endl;
  }

  static int count() { return count; }

private:
  std::vector<int> data_;

  static int count;
};

void print(const Foo& f) { std::cout << f.size() << std::endl; }

int Builder::count = 0;

int main()
{
  Foo f(std::vector<int>{1, 2, 3});
  std::cout << f.size() << " " << f.update() << std::endl;

  print(f);

  Builder b;

  b.add(1).add(2).add(3).build();
}
