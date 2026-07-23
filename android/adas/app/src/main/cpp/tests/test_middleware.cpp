#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "middleware/middleware.hpp"
#include "utils/logger.h"

namespace {

struct Ping {
  int id = 0;
  std::string payload;
};

class PubService : public adas::Service {
public:
  std::string_view getName() const override { return "pub"; }

  void configure() override {}

  void publishPing(int id, const std::string& payload)
  {
    Ping m{id, payload};
    publish("test/ping", m);
  }
};

class SubService : public adas::Service {
public:
  std::string_view getName() const override { return "sub"; }

  void configure() override
  {
    subscribe<Ping>("test/ping", [this](const Ping& m) {
      last_id_ = m.id;
      last_payload_ = m.payload;
      ++count_;
    });
  }

  void reset() override
  {
    count_ = 0;
    last_id_ = -1;
    last_payload_.clear();
  }

  int count() const { return count_.load(); }
  int lastId() const { return last_id_.load(); }
  std::string lastPayload() const { return last_payload_; }

private:
  std::atomic<int> count_{0};
  std::atomic<int> last_id_{-1};
  std::string last_payload_;
};

class TimerService : public adas::Service {
public:
  std::string_view getName() const override { return "timer"; }

  void configure() override
  {
    scheduleTimer(
        10, [this] { ++ticks_; }, "fast");
    scheduleTimer(
        50, [this] { ++slow_ticks_; }, "slow");
  }

  int ticks() const { return ticks_.load(); }
  int slowTicks() const { return slow_ticks_.load(); }

private:
  std::atomic<int> ticks_{0};
  std::atomic<int> slow_ticks_{0};
};

}  // namespace

TEST(MiddlewareTest, StartStopRealtime)
{
  auto pub = std::make_shared<PubService>();
  auto sub = std::make_shared<SubService>();
  auto mgr =
      std::make_shared<adas::Middleware>(adas::Middleware::Mode::RealTime, std::vector<adas::ServicePtr>{pub, sub});

  EXPECT_EQ(2u, mgr->startAll());
  EXPECT_TRUE(mgr->isRunning(pub));
  EXPECT_TRUE(mgr->isRunning(sub));

  pub->publishPing(7, "hello");
  for (int i = 0; i < 50 && sub->count() == 0; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

  EXPECT_EQ(1, sub->count());
  EXPECT_EQ(7, sub->lastId());
  EXPECT_EQ("hello", sub->lastPayload());

  EXPECT_EQ(2u, mgr->stopAll());
  EXPECT_FALSE(mgr->isRunning(pub));
}

TEST(MiddlewareTest, TimersRealtime)
{
  auto svc = std::make_shared<TimerService>();
  auto mgr = std::make_shared<adas::Middleware>(adas::Middleware::Mode::RealTime, std::vector<adas::ServicePtr>{svc});
  ASSERT_EQ(1u, mgr->startAll());
  std::this_thread::sleep_for(std::chrono::milliseconds(55));
  EXPECT_GE(svc->ticks(), 3);
  mgr->stopAll();
}

TEST(MiddlewareTest, SimulatedStep)
{
  auto pub = std::make_shared<PubService>();
  auto sub = std::make_shared<SubService>();
  auto mgr =
      std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated, std::vector<adas::ServicePtr>{pub, sub});

  mgr->setTime(0);
  pub->publishPing(1, "sim");
  EXPECT_EQ(0, sub->count());
  mgr->step();
  EXPECT_EQ(1, sub->count());
  EXPECT_EQ(1, sub->lastId());
}

TEST(MiddlewareTest, SimulatedTimers)
{
  auto svc = std::make_shared<TimerService>();
  auto mgr = std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated, std::vector<adas::ServicePtr>{svc});

  mgr->setTime(0);
  mgr->step();
  EXPECT_EQ(0, svc->ticks());
  mgr->setTime(10'000);
  mgr->step();
  EXPECT_EQ(1, svc->ticks());
  mgr->setTime(30'000);
  mgr->step();
  EXPECT_GE(svc->ticks(), 3);
}

TEST(MiddlewareTest, InternalTopicPublishing)
{
  class CanMsg {
  public:
    uint32_t address = 0;
  };

  class Publisher : public adas::Service {
  public:
    void configure() override {}
    void send()
    {
      CanMsg m;
      m.address = 0xFD;
      publish("sensors/can", m);
    }
  };

  class Consumer : public adas::Service {
  public:
    void configure() override
    {
      subscribe<CanMsg>("sensors/can", [this](const CanMsg& m) {
        last_ = m.address;
        ++n_;
      });
    }
    std::atomic<int> n_{0};
    std::atomic<uint32_t> last_{0};
  };

  auto pub = std::make_shared<Publisher>();
  auto cons = std::make_shared<Consumer>();
  auto mgr =
      std::make_shared<adas::Middleware>(adas::Middleware::Mode::RealTime, std::vector<adas::ServicePtr>{pub, cons});
  mgr->startAll();
  pub->send();
  for (int i = 0; i < 50 && cons->n_ == 0; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  EXPECT_EQ(1, cons->n_.load());
  EXPECT_EQ(0xFDu, cons->last_.load());
  mgr->stopAll();
}

TEST(MiddlewareTest, RegisterService)
{
  auto mw = std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated);
  auto pub = mw->registerService<PubService>();
  auto sub = mw->registerService<SubService>();
  EXPECT_EQ(2u, mw->getServiceCount());

  mw->setTime(0);
  pub->publishPing(42, "reg");
  mw->step();
  EXPECT_EQ(1, sub->count());
  EXPECT_EQ(42, sub->lastId());
}

TEST(MiddlewareTest, BoundedSubscriptionDropsOldest)
{
  class SlowSub : public adas::Service {
  public:
    void configure() override
    {
      // capacity 2: publish 5 before step → keep last 2, drop 3
      subscribe<Ping>(
          "test/ping", [this](const Ping& m) { ids_.push_back(m.id); },
          /*queue_capacity=*/2);
    }
    std::vector<int> ids_;
  };

  auto mw = std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated);
  auto pub = mw->registerService<PubService>();
  auto sub = mw->registerService<SlowSub>();

  mw->setTime(0);
  for (int i = 1; i <= 5; ++i)
    pub->publishPing(i, "x");

  EXPECT_EQ(3u, mw->droppedTotal());
  mw->step();
  ASSERT_EQ(2u, sub->ids_.size());
  EXPECT_EQ(4, sub->ids_[0]);
  EXPECT_EQ(5, sub->ids_[1]);
}

TEST(MiddlewareTest, SnapshotStatsTracksTimers)
{
  auto mw = std::make_shared<adas::Middleware>(adas::Middleware::Mode::Simulated);
  auto svc = mw->registerService<TimerService>();
  mw->setTime(0);
  mw->step();
  mw->setTime(10'000);
  mw->step();
  mw->setTime(50'000);
  mw->step();

  const auto snap = mw->snapshotStats();
  EXPECT_EQ(1u, snap.services);
  ASSERT_FALSE(snap.services_timing.empty());
  EXPECT_GE(snap.services_timing[0].timers_fired, 3u);
  EXPECT_FLOAT_EQ(10.f, snap.services_timing[0].period_ms);
  ASSERT_EQ(2u, snap.services_timing[0].timers.size());
  // Order follows schedule order: fast then slow
  EXPECT_EQ("fast", snap.services_timing[0].timers[0].name);
  EXPECT_FLOAT_EQ(10.f, snap.services_timing[0].timers[0].period_ms);
  EXPECT_EQ("slow", snap.services_timing[0].timers[1].name);
  EXPECT_FLOAT_EQ(50.f, snap.services_timing[0].timers[1].period_ms);
  // Mixed periods must not falsely mark shortest timer as lagging when on time
  EXPECT_FALSE(snap.services_timing[0].timers[0].lagging);
  EXPECT_FALSE(snap.any_lagging);
}

TEST(MiddlewareTest, FullZmqIntegration)
{
  GTEST_SKIP() << "Needs rewrite for ZmqBridgeService(endpoint_in, endpoint_out)";
}

TEST(MiddlewareTest, RequestResponseBasic) { GTEST_SKIP() << "request/respond removed (no RPC)"; }

TEST(MiddlewareTest, RequestResponseTimeout) { GTEST_SKIP() << "request/respond removed (no RPC)"; }

TEST(MiddlewareTest, RequestResponseNoResponse) { GTEST_SKIP() << "request/respond removed (no RPC)"; }
