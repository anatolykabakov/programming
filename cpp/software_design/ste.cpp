#include "atom/replay/ste.h"

namespace replay
{

SingleThreadExecutor::SingleThreadExecutor()
{
    worker_ = std::thread(&SingleThreadExecutor::run, this);
}

void SingleThreadExecutor::run()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tasks_.empty() && !stop_.load())
        {
            cv_.wait(lock, [this]() { return !tasks_.empty() || stop_.load(); });
        }

        if (stop_.load() && tasks_.empty())
        {
            return;
        }

        if (tasks_.empty())
        {
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        auto top_task = tasks_.top();

        if (now >= top_task.time)
        {
            Task task = std::move(const_cast<Task&>(top_task));
            tasks_.pop();
            lock.unlock();

            task.task();

            if (task.periodic && !stop_.load())
            {
                task.time = std::chrono::steady_clock::now() + task.interval;
                {
                    std::lock_guard<std::mutex> lock_guard(mutex_);
                    tasks_.push(std::move(task));
                }
                cv_.notify_one();
            }
        }
        else
        {
            cv_.wait_until(lock, top_task.time);
        }
    }
}

void SingleThreadExecutor::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_.store(true);
    }
    cv_.notify_all();
    if (worker_.joinable())
    {
        worker_.join();
    }
}

SingleThreadExecutor::~SingleThreadExecutor()
{
    stop();
}

void SingleThreadExecutor::post(
    std::function<void()> task,
    std::chrono::milliseconds interval,
    bool periodic
)
{
    const auto execution_time = std::chrono::steady_clock::now();
    const std::uint64_t seq = ++next_sequence_;
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(Task{std::move(task), execution_time, interval, periodic, seq});
    cv_.notify_one();
}

}  // namespace replay
