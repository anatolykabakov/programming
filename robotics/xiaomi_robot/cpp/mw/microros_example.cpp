// Mirrors cpp/mw/example.cpp: each logical node is a \c Node (own queue + spinner + mgr in
// ctor).
#include "microros.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace
{
const std::size_t K_LISTENER_QUEUE_SIZE = 10;
const std::chrono::milliseconds K_DEMO_RUNTIME(450);
}  // namespace

class ListenerNode : public middleware::Node
{
public:
    explicit ListenerNode(middleware::MiddlewareManager& mgr) : Node(mgr) {}

    void Configure() override
    {
        Subscribe<std::string>(
            "demo/topic",
            K_LISTENER_QUEUE_SIZE,
            [this](const std::string& msg)
            { std::cout << "ListenerNode got: " << msg << std::endl; }
        );
    }
};

class TalkerNode : public middleware::Node
{
public:
    TalkerNode(middleware::MiddlewareManager& mgr, std::string topic)
        : Node(mgr)
        , topic_(std::move(topic))
    {
    }

    void Send(const std::string& msg)
    {
        Publish<std::string>(topic_, msg);
    }

    void Configure() override
    {
        ScheduleTimer(middleware::K_EXAMPLE_TIMER_INTERVAL, [this]() { Send("periodic ping"); });
    }

private:
    std::string topic_;
};

int main()
{
    middleware::MiddlewareManager mgr;

    std::shared_ptr<ListenerNode> listener = mgr.RegisterNode<ListenerNode>();
    std::shared_ptr<TalkerNode> talker = mgr.RegisterNode<TalkerNode>("demo/topic");
    mgr.Start();

    talker->Send("hello from TalkerNode");

    std::this_thread::sleep_for(K_DEMO_RUNTIME);

    mgr.Stop();

    std::cout << "done" << std::endl;
    return 0;
}
