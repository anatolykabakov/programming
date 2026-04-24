#include "callback_queue.h"

#include <utility>

namespace middleware
{

namespace
{
const std::chrono::milliseconds K_IDLE_WAIT(10);
}

CallbackQueue::CallbackQueue() : disabled_(false) {}

CallbackQueue::~CallbackQueue() = default;

void CallbackQueue::AddCallback(std::function<void()> cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (disabled_)
        {
            return;
        }
        queue_.push(std::move(cb));
    }
    ready_.notify_one();
}

bool CallbackQueue::CallOne()
{
    std::function<void()> work;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty() || disabled_)
        {
            return false;
        }
        work = std::move(queue_.front());
        queue_.pop();
    }
    if (work)
    {
        work();
    }
    return true;
}

bool CallbackQueue::CallOneFor(std::chrono::milliseconds wait)
{
    std::function<void()> work;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty() && !disabled_ && wait.count() > 0)
        {
            ready_.wait_for(lock, wait, [this] { return !queue_.empty() || disabled_; });
        }
        if (queue_.empty() || disabled_)
        {
            return false;
        }
        work = std::move(queue_.front());
        queue_.pop();
    }
    if (work)
    {
        work();
    }
    return true;
}

void CallbackQueue::CallAvailable()
{
    while (CallOne())
    {
    }
}

void CallbackQueue::Disable()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        disabled_ = true;
        while (!queue_.empty())
        {
            queue_.pop();
        }
    }
    ready_.notify_all();
}

std::size_t CallbackQueue::Size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

SingleThreadedSpinner::SingleThreadedSpinner(CallbackQueue& queue) : queue_(queue), stop_(false) {}

SingleThreadedSpinner::~SingleThreadedSpinner()
{
    RequestStop();
}

void SingleThreadedSpinner::Spin()
{
    if (t_.joinable())
    {
        return;
    }
    stop_.store(false);
    t_ = std::thread(
        [this]()
        {
            while (!stop_.load())
            {
                if (!queue_.CallOneFor(K_IDLE_WAIT))
                {
                    // idle
                }
            }
        }
    );
}

void SingleThreadedSpinner::SpinOnce()
{
    queue_.CallOne();
}

void SingleThreadedSpinner::SpinOnceDrain()
{
    queue_.CallAvailable();
}

void SingleThreadedSpinner::RequestStop()
{
    stop_.store(true);
    if (t_.joinable())
    {
        if (std::this_thread::get_id() == t_.get_id())
        {
            t_.detach();
            return;
        }
        t_.join();
    }
}

namespace detail
{

SubscriptionSlot::SubscriptionSlot(
    CallbackQueue* queue,
    std::size_t queue_size,
    std::function<void(const void*)> user_cb
)
    : queue_(queue)
    , queue_size_(queue_size)
    , user_cb_(std::move(user_cb))
    , drain_scheduled_(false)
{
}

bool SubscriptionSlot::DeliversTo(const CallbackQueue* q) const
{
    return queue_ == q;
}

void SubscriptionSlot::Enqueue(const std::shared_ptr<void>& msg)
{
    bool schedule = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        backlog_.push_back(msg);
        if (queue_size_ > 0)
        {
            while (backlog_.size() > queue_size_)
            {
                backlog_.pop_front();
            }
        }
        if (!drain_scheduled_)
        {
            drain_scheduled_ = true;
            schedule = true;
        }
    }
    if (schedule)
    {
        std::shared_ptr<SubscriptionSlot> self = shared_from_this();
        queue_->AddCallback([self]() { self->DrainStep(); });
    }
}

void SubscriptionSlot::ClearForShutdown()
{
    std::lock_guard<std::mutex> lk(mtx_);
    backlog_.clear();
    drain_scheduled_ = false;
}

void SubscriptionSlot::DrainStep()
{
    std::shared_ptr<void> msg;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (backlog_.empty())
        {
            drain_scheduled_ = false;
            return;
        }
        msg = std::move(backlog_.front());
        backlog_.pop_front();
    }
    if (user_cb_)
    {
        user_cb_(msg.get());
    }
    bool more = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!backlog_.empty())
        {
            more = true;
        }
        else
        {
            drain_scheduled_ = false;
        }
    }
    if (more)
    {
        std::shared_ptr<SubscriptionSlot> self = shared_from_this();
        queue_->AddCallback([self]() { self->DrainStep(); });
    }
}

}  // namespace detail

}  // namespace middleware
