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
  MOCK_METHOD0(isActive, bool());
  MOCK_METHOD0(GetId, std::string());
};

TEST(TimerTest, StartTest)
{
  MockTimer timer;
  EXPECT_CALL(timer, Start(0)).Times(1);
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
  MockTimer timer;
  EXPECT_CALL(timer, GetId()).WillRepeatedly(testing::Return(std::string("GetId")));
  EXPECT_EQ("GetId", timer.GetId());
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
