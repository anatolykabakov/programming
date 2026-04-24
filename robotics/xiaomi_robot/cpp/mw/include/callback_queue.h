#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace middleware
{

/// FIFO of callables, roughly analogous to \c ros::CallbackQueue.
class CallbackQueue
{
public:
    CallbackQueue();
    ~CallbackQueue();

    CallbackQueue(const CallbackQueue&) = delete;
    CallbackQueue& operator=(const CallbackQueue&) = delete;

    void AddCallback(std::function<void()> cb);
    bool CallOne();
    bool CallOneFor(std::chrono::milliseconds wait);
    void CallAvailable();
    void Disable();
    std::size_t Size() const;

private:
    using Queue = std::queue<std::function<void()>>;

    mutable std::mutex mutex_;
    std::condition_variable ready_;
    Queue queue_;
    bool disabled_;
};

/// Spinner bound to a single \c CallbackQueue (roscpp-style single-threaded spin).
class SingleThreadedSpinner
{
public:
    explicit SingleThreadedSpinner(CallbackQueue& queue);
    ~SingleThreadedSpinner();

    SingleThreadedSpinner(const SingleThreadedSpinner&) = delete;
    SingleThreadedSpinner& operator=(const SingleThreadedSpinner&) = delete;

    void Spin();
    void SpinOnce();
    void SpinOnceDrain();
    void RequestStop();

private:
    CallbackQueue& queue_;
    std::atomic<bool> stop_;
    std::thread t_;
};

namespace detail
{

class SubscriptionSlot : public std::enable_shared_from_this<SubscriptionSlot>
{
public:
    SubscriptionSlot(
        CallbackQueue* queue,
        std::size_t queue_size,
        std::function<void(const void*)> user_cb
    );

    bool DeliversTo(const CallbackQueue* q) const;
    void Enqueue(const std::shared_ptr<void>& msg);
    void ClearForShutdown();

private:
    void DrainStep();

    CallbackQueue* queue_;
    std::size_t queue_size_;
    std::function<void(const void*)> user_cb_;
    std::deque<std::shared_ptr<void>> backlog_;
    std::mutex mtx_;
    bool drain_scheduled_;
};

}  // namespace detail

}  // namespace middleware
