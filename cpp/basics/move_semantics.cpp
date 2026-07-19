#include <iostream>
#include <utility>
#include <vector>
#include <memory>

void fooCopy(std::string s) { std::cout << "foo(std::string s) " << s << std::endl; }

void fooRef(const std::string& s) { std::cout << "foo(const std::string &s) " << s << std::endl; }

void fooRValueRef(std::string&& s) { std::cout << "std::string &&s " << s << std::endl; }

struct Foo {
  Foo() = default;
  Foo(Foo&) { std::cout << "Foo(&)" << std::endl; }
  Foo& operator=(Foo&)
  {
    std::cout << "Foo& operator=Foo(&)" << std::endl;
    return *this;
  }
  Foo(std::vector<int> v) : data(std::move(v)) { std::cout << "ctor" << std::endl; }
  Foo(Foo&& other) noexcept : data(std::move(other.data)) { std::cout << "Foo(Foo&&)" << std::endl; }
  Foo& operator=(Foo&& other)
  {
    if (this == &other)
      return *this;
    data = std::move(other.data);
    std::cout << "Foo& operator=(Foo&&)" << std::endl;
    return *this;
  }

  std::vector<int> data;
};

Foo makeFoo()
{
  std::vector<int> v{1, 2, 3};
  return Foo(std::move(v));
}

template <typename T>
void foo(T& x)
{
  std::cout << "foo(T& x)" << std::endl;
}

template <typename T>
void foo(T&& x)
{
  std::cout << "foo(T&& x)" << std::endl;
}

template <typename T>
void pass(T&& x)
{
  foo(x);                   // foo(T&)
  foo(std::forward<T>(x));  // foo(T&)
  foo(std::move(x));        // foo(T&&)
}

int main()
{
  std::string lvalue = "hello";  // lvalue имеет имя, адрес
  fooCopy(lvalue);
  fooRef(lvalue);
  // fooRValueRef(lvalue); error

  fooRValueRef("temp");
  fooRValueRef(std::move(lvalue));
  std::cout << lvalue << " " << lvalue.size() << std::endl;  // нет гарантии

  Foo a;
  Foo b = std::move(a);  // Foo(Foo&&)
  Foo c({1, 2, 3});
  c = std::move(b);  // Foo& operator=(Foo&&)

  Foo d = makeFoo();  // Copy ellision / NRVO / RVO C++17 без копии

  std::vector<Foo> v;
  v.push_back(Foo{});  // Foo(Foo&&)

  std::vector<int> big(1'000'000, 42);
  auto v2 = std::move(big);              // cheap
  std::cout << big.size() << std::endl;  // 0

  int var = 1;
  pass(var);  // lvalue
  pass(1);    // rvalue -> forward -> foo(T&&)

  // return std::move(local)

  const Foo fc{};
  // Foo fcc = std::move(fc); error

  auto s = std::make_shared<int>(42);
  std::weak_ptr<int> w = s;
  std::cout << s.use_count() << std::endl;
  if (auto p = w.lock()) {
    std::cout << s.use_count() << " " << *p << std::endl;
  }
  s.reset();
  std::cout << w.expired() << " " << !w.lock() << std::endl;

  return 0;
}
