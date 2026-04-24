// Educational sketch: roscpp-like pipeline "I/O (mock) -> CallbackQueue -> spinner thread".
// Pure C++11, no ROS. Not production middleware.

#pragma once

#include "callback_queue.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace middleware
{

extern const std::chrono::milliseconds K_EXAMPLE_TIMER_INTERVAL;

class MiddlewareManager;

/// One logical ROS node: \c MiddlewareManager& for graph + \c Publish, own \c CallbackQueue, own
/// \c SingleThreadedSpinner, timers. Derived classes implement \c Configure() (subscriptions etc.).
class Node
{
public:
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    void Spin()
    {
        spinner_->Spin();
    }

    template <typename T>
    void Publish(const std::string& topic, const T& msg);

    template <typename T>
    void
    Subscribe(const std::string& topic, std::size_t queue_size, std::function<void(const T&)> cb);
    template <typename T>
    void Subscribe(const std::string& topic, std::function<void(const T&)> cb);

    void ScheduleTimer(std::chrono::milliseconds period, std::function<void()> cb);

protected:
    explicit Node(MiddlewareManager& mgr);

    virtual void Configure() = 0;

private:
    friend class MiddlewareManager;

    void Shutdown();

    MiddlewareManager& mgr_;
    CallbackQueue queue_;
    std::unique_ptr<SingleThreadedSpinner> spinner_;
    std::atomic<bool> shutdown_done_;
};

class MiddlewareManager
{
public:
    enum class Mode
    {
        RealTime,
        Sequential
    };

    explicit MiddlewareManager(Mode mode = Mode::RealTime);
    ~MiddlewareManager();

    MiddlewareManager(const MiddlewareManager&) = delete;
    MiddlewareManager& operator=(const MiddlewareManager&) = delete;

    template <typename T>
    void Publish(const std::string& topic, const T& msg)
    {
        std::shared_ptr<void> data = std::make_shared<T>(msg);
        PublishImpl(topic, data);
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> RegisterNode(Args&&... args)
    {
        std::shared_ptr<T> node = std::make_shared<T>(*this, std::forward<Args>(args)...);
        RegisterNodeImpl(node.get());
        node->Configure();
        return node;
    }

    void Start();
    void Stop();
    void Step();
    uint64_t Now() const;
    void SetTime(uint64_t t_us);

private:
    friend class Node;  // Subscribe / Publish path to graph

    std::unique_ptr<SingleThreadedSpinner> GetSpinner(CallbackQueue& queue) const
    {
        if (mode_ == Mode::Sequential)
        {
            // return std::unique_ptr<SequentialSpinner>(new SequentialSpinner(queue));
        }
        return std::unique_ptr<SingleThreadedSpinner>(new SingleThreadedSpinner(queue));
    }

    template <typename T>
    void Subscribe(
        CallbackQueue& node_cb_queue,
        const std::string& topic,
        std::size_t queue_size,
        std::function<void(const T&)> cb
    )
    {
        std::function<void(const void*)> typed_cb = [cb](const void* data)
        { cb(*static_cast<const T*>(data)); };
        SubscribeImpl(node_cb_queue, topic, queue_size, std::move(typed_cb));
    }
    void PublishImpl(const std::string& topic, std::shared_ptr<void> data);
    void SubscribeImpl(
        CallbackQueue& node_cb_queue,
        const std::string& topic,
        std::size_t queue_size,
        std::function<void(const void*)> cb
    );

    void RegisterNodeImpl(Node* n);
    void UnregisterNode(Node* n);
    void RemoveSubscriptionsUsingQueue(CallbackQueue* q);
    void ScheduleTimer(
        CallbackQueue& node_cb_queue,
        std::chrono::milliseconds period,
        std::function<void()> cb
    );
    void RemoveTimersUsingQueue(CallbackQueue* q);

    struct SequentialTimer
    {
        CallbackQueue* queue;
        uint64_t period_us;
        uint64_t next_fire_us;
        std::function<void()> cb;
    };

    struct RealtimeTimer
    {
        CallbackQueue* queue;
        std::atomic<bool> stop;
        std::thread t;
        explicit RealtimeTimer(CallbackQueue* q) : queue(q), stop(false) {}
    };

    std::mutex sub_mtx_;
    std::map<std::string, std::vector<std::shared_ptr<detail::SubscriptionSlot>>> subs_;

    std::mutex nodes_mtx_;
    std::vector<Node*> nodes_;
    Mode mode_;
    bool started_;
    std::atomic<uint64_t> sim_time_us_;

    std::mutex timers_mtx_;
    std::vector<SequentialTimer> sequential_timers_;
    std::vector<std::shared_ptr<RealtimeTimer>> realtime_timers_;
};

}  // namespace middleware

template <typename T>
inline void middleware::Node::Publish(const std::string& topic, const T& msg)
{
    mgr_.Publish(topic, msg);
}

template <typename T>
inline void middleware::Node::Subscribe(
    const std::string& topic,
    std::size_t queue_size,
    std::function<void(const T&)> cb
)
{
    mgr_.Subscribe(queue_, topic, queue_size, std::move(cb));
}

template <typename T>
inline void middleware::Node::Subscribe(const std::string& topic, std::function<void(const T&)> cb)
{
    mgr_.Subscribe(queue_, topic, 0, std::move(cb));
}
