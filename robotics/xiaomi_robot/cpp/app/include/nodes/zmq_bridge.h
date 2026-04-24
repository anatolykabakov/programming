#pragma once

#include "middleware.hpp"
#include "types.h"

#include <memory>
#include <string>

#include "robot_app.h"
#include <zmq.hpp>

namespace project
{
namespace nodes
{

class ZmqBridge : public middleware::Service
{
public:
    explicit ZmqBridge(const app::RobotApp::Config& config);

protected:
    void Configure() override;
    void Reset() override;

private:
    void OnReceiveTimer();
    void OnOccupancyMap(const types::OccupancyMap& map);

    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> publisher_;
    std::unique_ptr<zmq::socket_t> subscriber_;
    app::RobotApp::Config config_;
};

}  // namespace nodes
}  // namespace project
