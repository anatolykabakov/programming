#pragma once

#include "middleware.hpp"
#include "types.h"

namespace project
{
namespace nodes
{

class LogNode : public middleware::Service
{
public:
    LogNode() = default;
    ~LogNode() = default;

protected:
    void Configure() override;

private:
    void OnLaserData(const types::LaserData& scan);
    void OnOdometryData(const types::OdometryData& odom);
    void OnGyroData(const types::GyroData& gyro);
    void OnCmdVel(const types::CmdVel& cmd_vel);
};

}  // namespace nodes
}  // namespace project
