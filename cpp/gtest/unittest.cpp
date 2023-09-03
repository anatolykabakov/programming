#include <gtest/gtest.h>

class Foo {
public:
  Foo(int i) : i_{i} {}

  void increment() { ++i_; }

  int get() { return i_; }

private:
  int i_{0};
};

struct MyFixture : public testing::Test {
  void SetUp() { f_ = new Foo(0); }

  void TearDown() { delete f_; }

  Foo* f_;
};

int sum(int a, int b) { return a + b; }

TEST(TestGroupName, SumTest)
{
  EXPECT_EQ(10, sum(5, 5));
  ASSERT_EQ(10, sum(5, 5));
}

TEST_F(MyFixture, DefailtGetTest) { EXPECT_EQ(f_->get(), 0); }

TEST_F(MyFixture, IncrementTest)
{
  f_->increment();
  ASSERT_EQ(f_->get(), 1);
}
