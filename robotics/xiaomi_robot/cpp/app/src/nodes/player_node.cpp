#include "nodes/player_node.h"

#include "drivers/player_interface.h"
#include "robot_app.h"
#include "types.h"

#include <cstdint>
#include <cstdio>

namespace project
{
namespace nodes
{

namespace
{

constexpr uint64_t K_PLAYER_TIMER_INTERVAL_US = 100000;

}  // namespace

PlayerNode::PlayerNode(const app::RobotApp::Config& config) : config_(config) {}

void PlayerNode::Configure()
{
    client_ = drivers::CreatePlayerClientC(config_.host, config_.player_port);
    Subscribe<types::CmdVel>(
        "cmd_vel",
        [this](const types::CmdVel& cmd_vel) { OnCmdVel(cmd_vel); }
    );
    ScheduleTimer(K_PLAYER_TIMER_INTERVAL_US, [this]() { OnTimer(); });
}

void PlayerNode::OnTimer()
{
    if (!client_ || !client_->UpdateRobotState())
    {
        std::fprintf(stderr, "player client read failed\n");
        return;
    }
    types::LaserData scan = client_->GetLaserData();
    scan.timestamp = Now();
    types::OdometryData pose = client_->GetOdometryData();
    pose.timestamp = Now();
    types::GyroData gyro = client_->GetGyroData();
    gyro.timestamp = Now();
    Publish<types::LaserData>("laser_data", scan);
    Publish<types::OdometryData>("odometry_data", pose);
    Publish<types::GyroData>("gyro_data", gyro);
}

void PlayerNode::OnCmdVel(const types::CmdVel& cmd_vel)
{
    if (!client_ || !client_->SetVelocityCommand(cmd_vel.vx, cmd_vel.vy, cmd_vel.w))
    {
        std::fprintf(stderr, "Failed to send velocity command\n");
    }
}

}  // namespace nodes
}  // namespace project
