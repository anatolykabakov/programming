#include "robot_app.h"

#include "middleware.hpp"
#include "nodes/log_node.h"
#include "nodes/mapping_node.h"
#ifdef XIAOMI_ROBOT_ENABLE_DRIVERS
#include "nodes/player_node.h"
#endif
#include "nodes/zmq_bridge.h"
#include "utils/utils.h"

#include <memory>
#include <string>

namespace project
{
namespace app
{

RobotApp::RobotApp(const Config& config) : config_(config)
{
    middleware_ =
        std::make_shared<middleware::ServiceManager>(middleware::ServiceManager::Mode::RealTime);
#ifdef XIAOMI_ROBOT_ENABLE_DRIVERS
    middleware_->RegisterService<nodes::PlayerNode>(config);
#endif
    middleware_->RegisterService<nodes::MappingNode>();
    middleware_->RegisterService<nodes::LogNode>();
    middleware_->RegisterService<nodes::ZmqBridge>(config);
}

RobotApp::~RobotApp() {}

void RobotApp::Start()
{
    middleware_->StartAll();
    utils::WaitForSignal();
    middleware_->StopAll();
}

RobotApp::Config RobotApp::Config::LoadFromJson(const std::string& config_path)
{
    auto root = utils::LoadJsonConfig(config_path);
    Config config;
    config.host = root["host"].asString();
    if (root.isMember("player_port"))
    {
        config.player_port = root["player_port"].asInt();
    }
    else if (root.isMember("playerport"))
    {
        config.player_port = root["playerport"].asInt();
    }
    config.zmq_sub_port = root["zmq_sub_port"].asInt();
    config.zmq_pub_port = root["zmq_pub_port"].asInt();
    return config;
}

}  // namespace app
}  // namespace project
