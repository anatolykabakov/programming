// ref https://spin.atomicobject.com/2013/04/16/googlemock-custom-return-values/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct ITimer {
  virtual void Start(int) = 0;
  virtual void Stop() = 0;
  virtual bool isActive() = 0;
  virtual std::string GetId() = 0;
  virtual ~ITimer() = default;
};

struct MockTimer : ITimer {
  MOCK_METHOD1(Start, void(int));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD(bool, isActive, (), (override));
  MOCK_METHOD(std::string, GetId, (), (override));
};

TEST(TimerTest, StartTest)
{
  MockTimer timer;
  EXPECT_CALL(timer, Start(::testing::_)).Times(1);
  timer.Start(0);
}

TEST(TimerTest, isActiveTest)
{
  MockTimer timer;
  EXPECT_CALL(timer, isActive()).Times(testing::AnyNumber());
  timer.isActive();
}

TEST(TimerTest, GetIdTest)
{
  ::testing::NiceMock<MockTimer> timer;  // NiceMock used to supress gtest warnings
  ON_CALL(timer, GetId()).WillByDefault(testing::Return(std::string("1")));
  EXPECT_EQ("1", timer.GetId());
}

TEST(TimerTest, isActiveValueTest)
{
  MockTimer timer;
  EXPECT_CALL(timer, isActive())
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Return(false))
      .WillRepeatedly(testing::Return(true));
  EXPECT_TRUE(timer.isActive());
  EXPECT_FALSE(timer.isActive());
  EXPECT_TRUE(timer.isActive());
  EXPECT_TRUE(timer.isActive());
}
