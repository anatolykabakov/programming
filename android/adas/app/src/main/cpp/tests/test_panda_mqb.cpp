#include <cstddef>

#include <gtest/gtest.h>

#include "panda/health.h"
#include "volkswagen/mqb_car_state_decoder.h"
#include "volkswagen/panda_safety_supervisor.h"

using volkswagen::cruiseAvailableFromTsk;
using volkswagen::cruiseEngagedFromTsk;
using volkswagen::isAllowedMqbRxAddress;
using volkswagen::PandaSafetySupervisor;

TEST(MqbDecoder, AllowlistContainsCoreIds)
{
  EXPECT_TRUE(isAllowedMqbRxAddress(0x086));
  EXPECT_TRUE(isAllowedMqbRxAddress(0x09F));
  EXPECT_TRUE(isAllowedMqbRxAddress(0x120));
  EXPECT_TRUE(isAllowedMqbRxAddress(0x126));
  EXPECT_FALSE(isAllowedMqbRxAddress(0x001));
}

TEST(MqbDecoder, TskCruiseMapping)
{
  EXPECT_FALSE(cruiseEngagedFromTsk(2));
  EXPECT_TRUE(cruiseAvailableFromTsk(2));
  EXPECT_TRUE(cruiseEngagedFromTsk(3));
  EXPECT_TRUE(cruiseEngagedFromTsk(4));
  EXPECT_TRUE(cruiseEngagedFromTsk(5));
  EXPECT_FALSE(cruiseEngagedFromTsk(0));
  EXPECT_FALSE(cruiseAvailableFromTsk(0));
}

TEST(PandaSafety, IgnitionStickyHysteresisAndDebounce)
{
  PandaSafetySupervisor s;

  EXPECT_TRUE(s.updateIgnitionSticky(false, 12000, 0));

  EXPECT_TRUE(s.updateIgnitionSticky(false, 11000, 1000));

  EXPECT_TRUE(s.updateIgnitionSticky(false, 10000, 2000));
  EXPECT_TRUE(s.updateIgnitionSticky(false, 10000, 4000));
  EXPECT_FALSE(s.updateIgnitionSticky(false, 10000, 5500));

  EXPECT_TRUE(s.updateIgnitionSticky(true, 10000, 6000));
}

TEST(PandaSafety, ConstantsMatchContract)
{
  using C = volkswagen::MqbSafetyConstants;
  EXPECT_EQ(C::kVolkswagen, 15);
  EXPECT_EQ(C::kNoOutput, 19);
  EXPECT_EQ(C::kIgnVoltageOnMv, 11500u);
  EXPECT_EQ(C::kIgnVoltageOffMv, 10500u);
  EXPECT_EQ(C::kIgnOffDebounceMs, 3000);
  EXPECT_EQ(C::kSafetyRetryBaseMs, 1000);
  EXPECT_EQ(C::kSafetyRetryMaxMs, 10000);
}

TEST(PandaHealth, PacketLayoutMatchesFirmwareV16)
{
  EXPECT_EQ(HEALTH_PACKET_VERSION, 16);
  EXPECT_EQ(offsetof(health_t, faults_pkt), 28u);
  EXPECT_EQ(offsetof(health_t, car_harness_status_pkt), 35u);
  EXPECT_EQ(offsetof(health_t, safety_mode_pkt), 36u);
  EXPECT_EQ(offsetof(health_t, power_save_enabled_pkt), 40u);
  EXPECT_EQ(sizeof(health_t), 58u);
}
