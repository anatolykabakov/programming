#include <iostream>
#include <memory>
#include <cassert>

struct Resource {
  Resource() { std::cout << "ctor" << std::endl; }
  ~Resource() { std::cout << "dtoc" << std::endl; }

  void foo() const { std::cout << "hello" << std::endl; }
};

void passByRef(const std::unique_ptr<Resource>& r) { r->foo(); }

void passByOwnership(std::unique_ptr<Resource> r) { r->foo(); }

struct Base {
  virtual void foo() { std::cout << "Foo::foo" << std::endl; }
  virtual ~Base() = default;  // UB on delete through base ptr
};

struct Child : public Base {
  void foo() override { std::cout << "Child::foo" << std::endl; }
};

void passByRef(std::shared_ptr<Resource>& r) { std::cout << r.use_count() << std::endl; }

void passByValue(std::shared_ptr<Resource> r) { std::cout << r.use_count() << std::endl; }

struct Bar {
  Bar(std::shared_ptr<Resource> r) : r_(std::move(r))
  {
    std::cout << r_.use_count() << std::endl;  // 2
  }

  std::shared_ptr<Resource> r_;
};

struct NodeBad {
  std::shared_ptr<NodeBad> next;
  ~NodeBad() { std::cout << "NodeBad dtoc" << std::endl; }
};

struct NodeGood {
  std::weak_ptr<NodeGood> next;
  ~NodeGood() { std::cout << "NodeGood dtoc" << std::endl; }
};

struct Foo {
  Foo() { std::cout << "Foo ctor" << std::endl; }
  virtual ~Foo() { std::cout << "Foo dtor" << std::endl; }
};

struct Bar2 : public Foo {
  Bar2() { std::cout << "Bar2 ctor" << std::endl; }
  ~Bar2() { std::cout << "Bar2 dtor" << std::endl; }
};

int main()
{
  std::unique_ptr<Resource> p = std::make_unique<Resource>();
  // auto p1 = p; // error
  auto p2 = std::move(p);  // transfer
  assert(p == nullptr);

  passByRef(p2);
  passByOwnership(std::move(p2));
  assert(p2 == nullptr);

  std::unique_ptr<Base> c = std::make_unique<Child>();
  c->foo();

  auto s1 = std::make_shared<Resource>();
  std::cout << s1.use_count() << std::endl;  // 1

  {
    auto s2 = s1;
    std::cout << s2.use_count() << std::endl;  // 2
  }

  std::cout << s1.use_count() << std::endl;  // 1

  passByRef(s1);    // 1
  passByValue(s1);  // 2

  std::cout << s1.use_count() << std::endl;  // 1

  Bar b(s1);

  std::cout << s1.use_count() << std::endl;  // 2

  {
    auto a = std::make_shared<NodeBad>();  // use_count = 1
    auto b = std::make_shared<NodeBad>();  // use_count = 1
    a->next = b;                           // a 1 b 2
    b->next = a;                           // a 2 b 2
  }                                        // A 1 B 1 не уничтаются --> учетчка!!!

  {
    auto a = std::make_shared<NodeGood>();
    auto b = std::make_shared<NodeGood>();
    a->next = b;  // a 1 weak 1
    b->next = a;
  }

  {
    std::unique_ptr<Foo> f = std::make_unique<Bar2>();
  }  // Bar2 не вызывается -- утечка!!

  auto fclose_del = [](FILE* f) {
    if (f)
      fclose(f);
  };
  std::unique_ptr<FILE, decltype(fclose_del)> uf(fopen("a.txt", "r"), fclose_del);
}
