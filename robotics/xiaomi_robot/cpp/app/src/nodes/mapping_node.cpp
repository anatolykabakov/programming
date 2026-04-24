#include "nodes/mapping_node.h"

namespace project
{
namespace nodes
{

MappingNode::MappingNode()
{
    occupancy_mapping_.reset(new algo::OccupancyMapping());
}

void MappingNode::Configure()
{
    Subscribe<types::LaserData>(
        "laser_data",
        [this](const types::LaserData& laser_data) { OnLaserData(laser_data); }
    );
    Subscribe<types::OdometryData>(
        "odometry_data",
        [this](const types::OdometryData& odometry_data) { OnOdometryData(odometry_data); }
    );

    ScheduleTimer(1000000, [this]() { OnTimer(); });
}

void MappingNode::OnLaserData(const types::LaserData& laser_data)
{
    occupancy_mapping_->UpdateScan(laser_data);
}

void MappingNode::OnOdometryData(const types::OdometryData& odometry_data)
{
    occupancy_mapping_->UpdateOdom(odometry_data);
}

void MappingNode::OnTimer()
{
    types::OccupancyMap occupancy_map = occupancy_mapping_->UpdateMap();
    occupancy_map.timestamp = Now();
    Publish<types::OccupancyMap>("occupancy_map", occupancy_map);
}
}  // namespace nodes
}  // namespace project
