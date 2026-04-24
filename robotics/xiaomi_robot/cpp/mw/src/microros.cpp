#include "microros.h"

#include <algorithm>
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

const std::chrono::milliseconds K_EXAMPLE_TIMER_INTERVAL(100);

Node::Node(MiddlewareManager& mgr)
    : mgr_(mgr)
    , spinner_(mgr.GetSpinner(queue_))
    , shutdown_done_(false)
{
}

Node::~Node()
{
    mgr_.RemoveSubscriptionsUsingQueue(&queue_);
    Shutdown();
    mgr_.UnregisterNode(this);
}

void Node::ScheduleTimer(std::chrono::milliseconds period, std::function<void()> cb)
{
    mgr_.ScheduleTimer(queue_, period, std::move(cb));
}

void Node::Shutdown()
{
    if (shutdown_done_.exchange(true))
    {
        return;
    }
    spinner_->RequestStop();
    mgr_.RemoveTimersUsingQueue(&queue_);
    queue_.Disable();
}

MiddlewareManager::MiddlewareManager(Mode mode) : mode_(mode), started_(false), sim_time_us_(0) {}

MiddlewareManager::~MiddlewareManager()
{
    Stop();
}

void MiddlewareManager::RegisterNodeImpl(Node* n)
{
    if (!n)
    {
        return;
    }
    bool should_start = false;
    {
        std::lock_guard<std::mutex> lock(nodes_mtx_);
        nodes_.push_back(n);
        should_start = started_;
    }
    if (should_start)
    {
        if (mode_ == Mode::RealTime)
        {
            n->Spin();
        }
    }
}

void MiddlewareManager::UnregisterNode(Node* n)
{
    if (!n)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(nodes_mtx_);
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), n), nodes_.end());
}

void MiddlewareManager::RemoveSubscriptionsUsingQueue(CallbackQueue* q)
{
    if (!q)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sub_mtx_);
    for (std::map<std::string, std::vector<std::shared_ptr<detail::SubscriptionSlot>>>::iterator
             it = subs_.begin();
         it != subs_.end();)
    {
        std::vector<std::shared_ptr<detail::SubscriptionSlot>>& vec = it->second;
        vec.erase(
            std::remove_if(
                vec.begin(),
                vec.end(),
                [q](const std::shared_ptr<detail::SubscriptionSlot>& s)
                { return s && s->DeliversTo(q); }
            ),
            vec.end()
        );
        if (vec.empty())
        {
            it = subs_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void MiddlewareManager::ScheduleTimer(
    CallbackQueue& node_cb_queue,
    std::chrono::milliseconds period,
    std::function<void()> cb
)
{
    if (period.count() <= 0)
    {
        return;
    }
    if (mode_ == Mode::Sequential)
    {
        SequentialTimer timer = {
            &node_cb_queue,
            static_cast<uint64_t>(period.count()) * 1000ULL,
            Now() + static_cast<uint64_t>(period.count()) * 1000ULL,
            std::move(cb)};
        std::lock_guard<std::mutex> lock(timers_mtx_);
        sequential_timers_.push_back(std::move(timer));
        return;
    }

    std::shared_ptr<RealtimeTimer> timer = std::make_shared<RealtimeTimer>(&node_cb_queue);
    timer->t = std::thread(
        [timer, period, cb]()
        {
            while (!timer->stop.load())
            {
                std::this_thread::sleep_for(period);
                if (timer->stop.load())
                {
                    break;
                }
                timer->queue->AddCallback(cb);
            }
        }
    );
    std::lock_guard<std::mutex> lock(timers_mtx_);
    realtime_timers_.push_back(timer);
}

void MiddlewareManager::RemoveTimersUsingQueue(CallbackQueue* q)
{
    if (!q)
    {
        return;
    }

    std::vector<std::shared_ptr<RealtimeTimer>> to_join;
    {
        std::lock_guard<std::mutex> lock(timers_mtx_);
        sequential_timers_.erase(
            std::remove_if(
                sequential_timers_.begin(),
                sequential_timers_.end(),
                [q](const SequentialTimer& t) { return t.queue == q; }
            ),
            sequential_timers_.end()
        );
        for (std::size_t i = 0; i < realtime_timers_.size(); ++i)
        {
            if (realtime_timers_[i] && realtime_timers_[i]->queue == q)
            {
                realtime_timers_[i]->stop.store(true);
                to_join.push_back(realtime_timers_[i]);
            }
        }
        realtime_timers_.erase(
            std::remove_if(
                realtime_timers_.begin(),
                realtime_timers_.end(),
                [q](const std::shared_ptr<RealtimeTimer>& t) { return t && t->queue == q; }
            ),
            realtime_timers_.end()
        );
    }

    for (std::size_t i = 0; i < to_join.size(); ++i)
    {
        if (to_join[i] && to_join[i]->t.joinable())
        {
            to_join[i]->t.join();
        }
    }
}

void MiddlewareManager::SubscribeImpl(
    CallbackQueue& node_cb_queue,
    const std::string& topic,
    std::size_t queue_size,
    std::function<void(const void*)> cb
)
{
    std::shared_ptr<detail::SubscriptionSlot> slot =
        std::make_shared<detail::SubscriptionSlot>(&node_cb_queue, queue_size, std::move(cb));
    std::lock_guard<std::mutex> lock(sub_mtx_);
    subs_[topic].push_back(slot);
}

void MiddlewareManager::PublishImpl(const std::string& topic, std::shared_ptr<void> data)
{
    std::vector<std::shared_ptr<detail::SubscriptionSlot>> slots;
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        const std::map<std::string, std::vector<std::shared_ptr<detail::SubscriptionSlot>>>::
            const_iterator it = subs_.find(topic);
        if (it == subs_.end())
        {
            return;
        }
        slots = it->second;
    }
    for (std::size_t i = 0; i < slots.size(); ++i)
    {
        if (slots[i])
        {
            slots[i]->Enqueue(data);
        }
    }
}

void MiddlewareManager::Start()
{
    if (mode_ == Mode::Sequential)
    {
        std::lock_guard<std::mutex> lock(nodes_mtx_);
        started_ = true;
        return;
    }

    std::vector<Node*> copy;
    {
        std::lock_guard<std::mutex> lock(nodes_mtx_);
        if (started_)
        {
            return;
        }
        started_ = true;
        copy = nodes_;
    }
    for (std::size_t i = 0; i < copy.size(); ++i)
    {
        copy[i]->Spin();
    }
}

void MiddlewareManager::Stop()
{
    std::vector<Node*> copy;
    std::vector<std::shared_ptr<RealtimeTimer>> timers_copy;
    {
        std::lock_guard<std::mutex> lock(nodes_mtx_);
        started_ = false;
        copy.swap(nodes_);
    }
    {
        std::lock_guard<std::mutex> lock(timers_mtx_);
        for (std::size_t i = 0; i < realtime_timers_.size(); ++i)
        {
            if (realtime_timers_[i])
            {
                realtime_timers_[i]->stop.store(true);
            }
        }
        timers_copy.swap(realtime_timers_);
        sequential_timers_.clear();
    }
    for (std::size_t i = 0; i < timers_copy.size(); ++i)
    {
        if (timers_copy[i] && timers_copy[i]->t.joinable())
        {
            timers_copy[i]->t.join();
        }
    }
    for (std::size_t i = 0; i < copy.size(); ++i)
    {
        if (copy[i])
        {
            copy[i]->Shutdown();
        }
    }
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);
        for (std::map<std::string, std::vector<std::shared_ptr<detail::SubscriptionSlot>>>::iterator
                 it = subs_.begin();
             it != subs_.end();
             ++it)
        {
            for (std::size_t j = 0; j < it->second.size(); ++j)
            {
                if (it->second[j])
                {
                    it->second[j]->ClearForShutdown();
                }
            }
        }
        subs_.clear();
    }
}

uint64_t MiddlewareManager::Now() const
{
    if (mode_ == Mode::Sequential)
    {
        return sim_time_us_.load();
    }
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch()
    )
                                     .count());
}

void MiddlewareManager::SetTime(uint64_t t_us)
{
    if (mode_ == Mode::Sequential)
    {
        sim_time_us_.store(t_us);
    }
}

void MiddlewareManager::Step()
{
    std::vector<Node*> nodes_copy;
    {
        std::lock_guard<std::mutex> lock(nodes_mtx_);
        nodes_copy = nodes_;
    }

    bool work_done = false;
    do
    {
        work_done = false;
        if (mode_ == Mode::Sequential)
        {
            std::vector<std::pair<CallbackQueue*, std::function<void()>>> due;
            const uint64_t now = Now();
            {
                std::lock_guard<std::mutex> lock(timers_mtx_);
                for (std::size_t i = 0; i < sequential_timers_.size(); ++i)
                {
                    if (sequential_timers_[i].next_fire_us <= now)
                    {
                        due.push_back(
                            std::make_pair(sequential_timers_[i].queue, sequential_timers_[i].cb)
                        );
                        sequential_timers_[i].next_fire_us = now + sequential_timers_[i].period_us;
                    }
                }
            }
            for (std::size_t i = 0; i < due.size(); ++i)
            {
                if (due[i].first)
                {
                    due[i].first->AddCallback(due[i].second);
                    work_done = true;
                }
            }
        }

        for (std::size_t i = 0; i < nodes_copy.size(); ++i)
        {
            if (!nodes_copy[i])
            {
                continue;
            }
            while (nodes_copy[i]->queue_.CallOne())
            {
                work_done = true;
            }
        }
    } while (work_done);
}

}  // namespace middleware
