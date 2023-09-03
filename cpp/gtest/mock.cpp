#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct IFoo {
  virtual void foo(int a) = 0;
  virtual ~IFoo() = default;
};

struct Foo : IFoo {
  Foo() = default;
  ~Foo() = default;

  void foo(int a) override {}
};

struct App {
  App(IFoo* foo) : foo_{foo} {}
  void run() { foo_->foo(1); }

private:
  IFoo* foo_;
};

struct MockFoo : IFoo {
  ~MockFoo() = default;
  MOCK_METHOD1(foo, void(int));
};

TEST(FooMock, FooTest)
{
  MockFoo mock;
  App app(&mock);
  EXPECT_CALL(mock, foo(1)).Times(1);
  app.run();
}
