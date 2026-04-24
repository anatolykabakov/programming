#pragma once

#include <memory>

#include "algo/occupancy_mapping.h"
#include "middleware.hpp"
#include "types.h"

namespace project
{
namespace nodes
{

class MappingNode : public middleware::Service
{
public:
    MappingNode();
    ~MappingNode() = default;

protected:
    void Configure() override;

private:
    std::unique_ptr<algo::OccupancyMapping> occupancy_mapping_;

    void OnLaserData(const types::LaserData& laser_data);
    void OnOdometryData(const types::OdometryData& odometry_data);
    void OnTimer();
};
}  // namespace nodes
}  // namespace project
