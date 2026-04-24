#include "include/middleware.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace
{
constexpr uint64_t K_EXAMPLE_TIMER_INTERVAL_US = 100000;
}

class ListenerNode : public middleware::Service
{
public:
    ListenerNode() = default;

protected:
    void Configure() override
    {
        Subscribe<std::string>(
            "demo/topic",
            [this](const std::string& msg)
            { std::cout << "ListenerNode got: " << msg << std::endl; }
        );
    }
};

class TalkerNode : public middleware::Service
{
public:
    explicit TalkerNode(const std::string& topic) : topic_(topic) {}

    void Send(const std::string& msg)
    {
        Publish<std::string>(topic_, msg);
    }

protected:
    void Configure() override
    {
        // Demonstrates timer usage in simulated mode.
        ScheduleTimer(K_EXAMPLE_TIMER_INTERVAL_US, [this]() { Send("periodic ping"); });
    }

private:
    std::string topic_;
};

int main()
{
    middleware::ServiceManager manager(middleware::ServiceManager::Mode::Simulated);
    manager.RegisterService<ListenerNode>();
    auto talker = manager.RegisterService<TalkerNode>("demo/topic");

    talker->Send("hello from TalkerNode");
    manager.Step();
    manager.SetTime(K_EXAMPLE_TIMER_INTERVAL_US);
    manager.Step();  // fires timer once in simulated stepping
    return 0;
}
