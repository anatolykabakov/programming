#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "utils/lat_control_pid.h"
#include "utils/pure_pursuit.h"
#include "volkswagen/carcontroller.h"
#include "volkswagen/values.h"

using adas::LatControlPid;
using adas::PidController;
using adas::PurePursuit;
using adas::Vec2;
using volkswagen::applyDriverSteerTorqueLimits;
using volkswagen::CarControllerParams;

TEST(PidController, ProportionalResponse)
{
  PidController pid(0.5, 0.0, 0.0, 50.0);
  EXPECT_NEAR(pid.update(10.0), 1.0, 1e-9);
  pid.reset();
  EXPECT_NEAR(pid.update(1.0), 0.5, 1e-9);
}

TEST(PidController, IntegratorAntiWindupAndOverrideUnwind)
{
  PidController pid(0.0, 10.0, 0.0, 50.0);

  for (int i = 0; i < 100; ++i)
    pid.update(1.0);
  EXPECT_NEAR(pid.control(), 1.0, 1e-9);
  const double i_sat = pid.i();
  EXPECT_GT(i_sat, 0.0);

  for (int i = 0; i < 20; ++i)
    pid.update(1.0, 0.0, true);
  EXPECT_LT(std::abs(pid.i()), std::abs(i_sat));
}

TEST(LatControlPid, InactiveResets)
{
  LatControlPid lat(0.6, 0.0, 0.0, 50.0);
  auto on = lat.update(true, 20.0, 0.0, 5.0, false);
  EXPECT_TRUE(on.active);
  EXPECT_GT(on.steer_norm, 0.0);

  auto off = lat.update(false, 20.0, 0.0, 5.0, false);
  EXPECT_FALSE(off.active);
  EXPECT_NEAR(off.steer_norm, 0.0, 1e-12);
}

TEST(LatControlPid, FeedforwardUsesVSquared)
{
  LatControlPid lat(0.0, 0.0, 0.001, 50.0);

  auto r = lat.update(true, 10.0, 10.0, 10.0, false);
  EXPECT_NEAR(r.angle_error_deg, 0.0, 1e-12);
  EXPECT_NEAR(r.steer_norm, 1.0, 1e-9);
  EXPECT_NEAR(r.f, 1.0, 1e-9);
}

TEST(PurePursuit, StraightCenterlineNearZeroSteer)
{
  PurePursuit pp(0.4, 2.636, 1.4, 3.0, 20.0);
  std::vector<Vec2> poly;
  for (int i = 0; i <= 20; ++i)
    poly.push_back({static_cast<double>(i), 0.0});
  const auto r = pp.compute(poly, 10.0);
  ASSERT_TRUE(r.target_ego.has_value());
  EXPECT_NEAR(r.steer_rad, 0.0, 1e-6);
  EXPECT_NEAR(r.lookahead_m, 4.0, 1e-9);
}

TEST(PurePursuit, LeftOffsetProducesPositiveSteer)
{
  PurePursuit pp(0.4, 2.636, 1.4, 3.0, 20.0);
  std::vector<Vec2> poly;
  for (int i = 0; i <= 20; ++i)
    poly.push_back({static_cast<double>(i), 1.5});
  const auto r = pp.compute(poly, 10.0);
  ASSERT_TRUE(r.target_ego.has_value());
  EXPECT_GT(r.steer_rad, 0.0);
}

TEST(PurePursuit, EmptyPolylineNoTarget)
{
  PurePursuit pp;
  const auto r = pp.compute({}, 5.0);
  EXPECT_FALSE(r.target_ego.has_value());
  EXPECT_NEAR(r.steer_rad, 0.0, 1e-12);
}

TEST(SteerTorqueLimits, FirstStepFromZeroMatchesPandaRateUp)
{
  EXPECT_EQ(applyDriverSteerTorqueLimits(300, 0.f, 0), CarControllerParams::STEER_DELTA_UP);
  EXPECT_EQ(applyDriverSteerTorqueLimits(-300, 0.f, 0), -CarControllerParams::STEER_DELTA_UP);
}

TEST(SteerTorqueLimits, RateUpWhilePositive)
{
  const int last = 40;
  EXPECT_EQ(applyDriverSteerTorqueLimits(300, 0.f, last), last + CarControllerParams::STEER_DELTA_UP);
  EXPECT_EQ(applyDriverSteerTorqueLimits(0, 0.f, last), last - CarControllerParams::STEER_DELTA_DOWN);
}

TEST(SteerTorqueLimits, AbsoluteCapAtSteerMax)
{
  EXPECT_EQ(applyDriverSteerTorqueLimits(300, 0.f, 296), 300);
  EXPECT_EQ(applyDriverSteerTorqueLimits(400, 0.f, 300), 300);
}

TEST(SteerTorqueLimits, ConstantsMatchPandaSafetyContract)
{
  EXPECT_EQ(CarControllerParams::STEER_DELTA_UP, 4);
  EXPECT_EQ(CarControllerParams::STEER_DELTA_DOWN, 10);
  EXPECT_EQ(CarControllerParams::STEER_MAX, 300);
  EXPECT_EQ(CarControllerParams::STEER_DRIVER_ALLOWANCE, 80);
}
