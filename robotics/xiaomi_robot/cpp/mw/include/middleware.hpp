
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <pthread.h>

// ========== Unified Logging ==========
namespace middleware
{
namespace internal
{
static constexpr uint64_t K_MAX_TIMER_POLL_DELAY_US = 1000;
static constexpr int K_REALTIME_SCHED_PRIORITY = 98;

inline void LogInfo(const std::string& msg)
{
    std::cout << msg << std::endl;
}
inline void LogError(const std::string& msg)
{
    std::cerr << msg << std::endl;
}
}  // namespace internal

class ServiceManager;
class Service;

class IServiceContext
{
public:
    virtual ~IServiceContext() = default;

    template <typename T>
    void Publish(const std::string& topic, const T& msg);

    template <typename T>
    void Subscribe(
        const std::string& topic,
        const std::shared_ptr<Service>& service,
        std::function<void(const T&)> cb
    );

    virtual void ScheduleTimer(
        uint64_t interval_us,
        const std::shared_ptr<Service>& service,
        std::function<void()> cb
    ) = 0;
    virtual size_t GetQueueSize(const std::shared_ptr<Service>& service) const = 0;
    virtual uint64_t Now() const = 0;

protected:
    virtual void PublishImpl(const std::string& topic, std::shared_ptr<void> data) = 0;

    virtual void SubscribeImpl(
        const std::string& topic,
        const std::shared_ptr<Service>& service,
        std::function<void(const void*)> cb
    ) = 0;
};

class Service : public std::enable_shared_from_this<Service>
{
protected:
    IServiceContext* context_ = nullptr;

    template <typename T>
    void Publish(const std::string& topic, const T& msg);

    template <typename T>
    void Subscribe(const std::string& topic, std::function<void(const T&)> cb);

    void ScheduleTimer(uint64_t interval_us, std::function<void()> cb);

    size_t GetQueueSize() const;

    uint64_t Now() const;

    virtual void Configure() = 0;

    virtual void Reset();

    virtual void OnException(const std::exception& e, const std::string& context)
    {
        middleware::internal::LogError("Exception in " + context + ": " + e.what());
    }

    friend class ServiceManager;
};

class ServiceManager : public IServiceContext
{
public:
    enum class Mode
    {
        RealTime,
        Simulated
    };

    explicit ServiceManager(Mode mode) : mode_(mode), sim_time_us_(0) {}

    ServiceManager(Mode mode, const std::vector<std::shared_ptr<Service>>& services)
        : ServiceManager(mode)
    {
        for (const auto& service : services)
        {
            RegisterService(service);
        }
    }

    template <typename ServiceType, typename... Args>
    std::shared_ptr<ServiceType> RegisterService(Args&&... args)
    {
        static_assert(
            std::is_base_of<Service, ServiceType>::value,
            "RegisterService<ServiceType>: ServiceType must derive from middleware::Service"
        );
        auto service = std::make_shared<ServiceType>(std::forward<Args>(args)...);
        RegisterService(service);
        return service;
    }

    ~ServiceManager()
    {
        StopAll();
    }

    bool Stop(const std::shared_ptr<Service>& service)
    {
        if (!service)
        {
            middleware::internal::LogError("Warning: stop() called with nullptr");
            return false;
        }

        if (mode_ != Mode::RealTime)
        {
            return false;
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            middleware::internal::LogError("Warning: stop() called with unregistered service");
            return false;
        }

        auto& ctx = it->second;
        if (ctx.running)
        {
            ctx.running = false;
            ctx.cv.notify_one();

            if (ctx.thread.joinable())
            {
                ctx.thread.join();
            }
            return true;
        }

        return false;
    }

    bool Start(const std::shared_ptr<Service>& service)
    {
        if (!service)
        {
            middleware::internal::LogError("Warning: start() called with nullptr");
            return false;
        }

        if (mode_ != Mode::RealTime)
        {
            return false;
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            middleware::internal::LogError("Warning: start() called with unregistered service");
            return false;
        }

        auto& ctx = it->second;
        if (!ctx.running)
        {
            service->Reset();
            ctx.running = true;

            ctx.thread = std::thread([this, service]() { RunService(service); });
            return true;
        }
        return false;
    }

    size_t StartAll()
    {
        if (mode_ != Mode::RealTime)
        {
            middleware::internal::LogError("Warning: startAll() only works in RealTime mode");
            return 0;
        }

        size_t started_count = 0;
        for (const auto& service : services_)
        {
            if (Start(service))
            {
                ++started_count;
            }
        }
        return started_count;
    }

    size_t StopAll()
    {
        if (mode_ != Mode::RealTime)
        {
            return 0;
        }

        size_t stopped_count = 0;
        for (const auto& service : services_)
        {
            if (Stop(service))
            {
                ++stopped_count;
            }
        }
        return stopped_count;
    }

    bool IsRunning(const std::shared_ptr<Service>& service)
    {
        if (!service)
        {
            return false;
        }
        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            return false;
        }
        return it->second.running;
    }

    size_t GetRunningCount() const
    {
        size_t count = 0;
        for (const auto& kv : contexts_)
        {
            if (kv.second.running)
            {
                ++count;
            }
        }
        return count;
    }

    size_t GetServiceCount() const
    {
        return services_.size();
    }

    struct ServiceStatistics
    {
        uint64_t messages_processed;
        uint64_t timers_fired;
        uint64_t exceptions_caught;
        uint64_t total_processing_time_us;
        uint64_t avg_processing_time_us;
    };

    bool GetServiceStats(const std::shared_ptr<Service>& service, ServiceStatistics& out_stats)
        const
    {
        if (!service)
        {
            return false;
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            return false;
        }

        const auto& stats = it->second.stats;
        uint64_t msg_count = stats.messages_processed.load();

        out_stats = ServiceStatistics{
            msg_count,
            stats.timers_fired.load(),
            stats.exceptions_caught.load(),
            stats.total_processing_time_us.load(),
            msg_count > 0 ? stats.total_processing_time_us.load() / msg_count : 0};
        return true;
    }

    void PrintStats() const
    {
        middleware::internal::LogInfo("\n=== service Statistics ===");
        middleware::internal::LogInfo("Total Services: " + std::to_string(services_.size()));
        middleware::internal::LogInfo("Running Services: " + std::to_string(GetRunningCount()));

        size_t total_messages = 0;
        size_t total_timers = 0;

        for (const auto& kv : contexts_)
        {
            const auto& stats = kv.second.stats;
            uint64_t msg = stats.messages_processed.load();
            uint64_t tim = stats.timers_fired.load();
            uint64_t exc = stats.exceptions_caught.load();

            total_messages += msg;
            total_timers += tim;

            if (msg > 0 || tim > 0)
            {
                std::ostringstream oss;
                oss << "  service " << kv.first << ": msg=" << msg << " timers=" << tim;
                if (exc > 0) oss << " exceptions=" << exc;
                middleware::internal::LogInfo(oss.str());
            }
        }

        middleware::internal::LogInfo(
            "Total: " + std::to_string(total_messages) + " messages, " +
            std::to_string(total_timers) + " timers fired"
        );
    }

    void SetTime(uint64_t t_us)
    {
        if (mode_ == Mode::Simulated)
        {
            sim_time_us_ = t_us;
        }
    }

    void Step()
    {
        bool work_exists;
        do
        {
            work_exists = false;
            for (auto& kv : contexts_)
            {
                if (ProcessMessages(kv.second))
                {
                    work_exists = true;
                }
            }
        } while (work_exists);

        uint64_t t = Now();
        for (auto& kv : contexts_)
        {
            ProcessTimers(kv.second, t);
        }
    }

    void SubscribeImpl(
        const std::string& topic,
        const std::shared_ptr<Service>& service,
        std::function<void(const void*)> cb
    ) override
    {
        if (!service)
        {
            throw std::invalid_argument("subscribe: nullptr service");
        }

        std::lock_guard<std::mutex> lk(sub_mtx_);
        subscriptions_[topic].push_back(Subscription{service, cb});
    }

    void ScheduleTimer(
        uint64_t interval_us,
        const std::shared_ptr<Service>& service,
        std::function<void()> cb
    ) override
    {
        if (!service)
        {
            throw std::invalid_argument("scheduleTimer: nullptr service");
        }
        if (interval_us == 0)
        {
            throw std::invalid_argument("scheduleTimer: interval_us must be > 0");
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            throw std::runtime_error("scheduleTimer: service not registered");
        }

        auto& ctx = it->second;
        std::lock_guard<std::mutex> tl(ctx.t_mtx);
        ctx.timers.push_back({interval_us, Now() + interval_us, cb});
    }

    size_t GetQueueSize(const std::shared_ptr<Service>& service) const override
    {
        if (!service)
        {
            throw std::invalid_argument("getQueueSize: nullptr service");
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            throw std::runtime_error("getQueueSize: service not registered");
        }

        const auto& ctx = it->second;
        std::lock_guard<std::mutex> ql(ctx.q_mtx);
        return ctx.queue.size();
    }

    uint64_t Now() const override
    {
        if (mode_ == Mode::RealTime)
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch()
            )
                .count();
        }
        return sim_time_us_;
    }

protected:
    void PublishImpl(const std::string& topic, std::shared_ptr<void> data_ptr) override
    {
        // LOCK ORDER: 1. sub_mtx_ -> 2. q_mtx (per-context)
        std::lock_guard<std::mutex> lk(sub_mtx_);
        auto it = subscriptions_.find(topic);
        if (it == subscriptions_.end())
        {
            return;
        }

        for (const auto& sub : it->second)
        {
            Service* key = sub.service.get();
            auto ctx_it = contexts_.find(key);
            if (ctx_it == contexts_.end())
            {
                continue;
            }

            auto& ctx = ctx_it->second;
            {
                std::lock_guard<std::mutex> ql(ctx.q_mtx);
                auto cb_copy = sub.cb;
                ctx.queue.push([cb_copy, data_ptr]() { cb_copy(data_ptr.get()); });
            }

            if (mode_ == Mode::RealTime)
            {
                ctx.cv.notify_one();
            }
        }
    }

private:
    void RegisterService(const std::shared_ptr<Service>& service)
    {
        if (!service)
        {
            throw std::invalid_argument("ServiceManager: nullptr service provided");
        }

        Service* key = service.get();
        if (contexts_.find(key) != contexts_.end())
        {
            throw std::invalid_argument("ServiceManager: service already registered");
        }

        services_.push_back(service);
        contexts_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(key),
            std::forward_as_tuple(service)
        );
        service->context_ = this;
        service->Configure();
    }

    struct Subscription
    {
        std::shared_ptr<Service> service;
        std::function<void(const void*)> cb;
    };

    struct Timer
    {
        uint64_t interval, next;
        std::function<void()> cb;
    };

    struct ServiceStats
    {
        std::atomic<uint64_t> messages_processed{0};
        std::atomic<uint64_t> timers_fired{0};
        std::atomic<uint64_t> exceptions_caught{0};
        std::atomic<uint64_t> total_processing_time_us{0};
    };

    struct Context
    {
        std::shared_ptr<Service> service;
        std::queue<std::function<void()>> queue;
        mutable std::mutex q_mtx;
        std::condition_variable cv;
        std::vector<Timer> timers;
        mutable std::mutex t_mtx;
        std::thread thread;
        std::atomic<bool> running{false};

        ServiceStats stats;

        explicit Context(std::shared_ptr<Service> ptr) : service(std::move(ptr)), running(false) {}
    };

    bool ProcessMessages(Context& ctx)
    {
        std::queue<std::function<void()>> qcopy;
        {
            std::lock_guard<std::mutex> ql(ctx.q_mtx);
            if (ctx.queue.empty())
            {
                return false;
            }
            std::swap(qcopy, ctx.queue);
        }

        size_t msg_count = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (!qcopy.empty())
        {
            try
            {
                qcopy.front()();
                ++msg_count;
            }
            catch (const std::exception& e)
            {
                ctx.stats.exceptions_caught++;
                if (ctx.service)
                {
                    ctx.service->OnException(e, "message callback");
                }
            }
            catch (...)
            {
                ctx.stats.exceptions_caught++;
                middleware::internal::LogError("Unknown exception in message callback");
            }
            qcopy.pop();
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        ctx.stats.messages_processed += msg_count;
        ctx.stats.total_processing_time_us += duration_us;

        return true;
    }

    void ProcessTimers(Context& ctx, uint64_t t)
    {
        std::lock_guard<std::mutex> tl(ctx.t_mtx);

        for (auto& tm : ctx.timers)
        {
            while (t >= tm.next)
            {
                try
                {
                    tm.cb();
                    ctx.stats.timers_fired++;
                }
                catch (const std::exception& e)
                {
                    ctx.stats.exceptions_caught++;
                    if (ctx.service)
                    {
                        ctx.service->OnException(e, "timer callback");
                    }
                }
                catch (...)
                {
                    ctx.stats.exceptions_caught++;
                    middleware::internal::LogError("Unknown exception in timer callback");
                }
                tm.next += tm.interval;
            }
        }
    }

    std::chrono::microseconds GetNextTimerDelay(const Context& ctx)
    {
        std::lock_guard<std::mutex> tl(ctx.t_mtx);

        if (ctx.timers.empty())
        {
            return std::chrono::milliseconds(1);
        }

        uint64_t current_time = Now();
        uint64_t next_timer_us = UINT64_MAX;

        for (const auto& tm : ctx.timers)
        {
            if (tm.next < next_timer_us)
            {
                next_timer_us = tm.next;
            }
        }

        if (next_timer_us <= current_time)
        {
            return std::chrono::microseconds(0);
        }

        uint64_t delay_us = next_timer_us - current_time;
        return std::chrono::microseconds(std::min(delay_us, internal::K_MAX_TIMER_POLL_DELAY_US));
    }

    void RunService(const std::shared_ptr<Service>& service)
    {
        sched_param sch_params;
        sch_params.sched_priority = internal::K_REALTIME_SCHED_PRIORITY;

        int result = pthread_setschedparam(pthread_self(), SCHED_RR, &sch_params);
        if (result != 0)
        {
            middleware::internal::LogError(
                "Warning: Failed to set real-time priority (error " + std::to_string(result) + ")"
            );
        }

        Service* key = service.get();
        auto it = contexts_.find(key);
        if (it == contexts_.end())
        {
            return;
        }

        auto& ctx = it->second;

        {
            std::lock_guard<std::mutex> tl(ctx.t_mtx);
            const auto curr_time = Now();
            for (auto& timer : ctx.timers)
            {
                timer.next = curr_time + timer.interval;
            }
        }

        ctx.running = true;

        while (ctx.running)
        {
            ProcessMessages(ctx);
            ProcessTimers(ctx, Now());

            auto timeout = GetNextTimerDelay(ctx);

            std::unique_lock<std::mutex> lk(ctx.q_mtx);
            ctx.cv.wait_for(lk, timeout, [&ctx]() { return !ctx.queue.empty() || !ctx.running; });
        }
    }

    Mode mode_;
    uint64_t sim_time_us_;

    std::vector<std::shared_ptr<Service>> services_;

    std::map<Service*, Context> contexts_;

    std::map<std::string, std::vector<Subscription>> subscriptions_;
    std::mutex sub_mtx_;
};

template <typename T>
inline void IServiceContext::Publish(const std::string& topic, const T& msg)
{
    auto data_ptr = std::make_shared<T>(msg);
    PublishImpl(topic, data_ptr);
}

template <typename T>
inline void IServiceContext::Subscribe(
    const std::string& topic,
    const std::shared_ptr<Service>& service,
    std::function<void(const T&)> cb
)
{
    auto typed_cb = [cb](const void* data) { cb(*static_cast<const T*>(data)); };
    SubscribeImpl(topic, service, typed_cb);
}

template <typename T>
inline void Service::Publish(const std::string& topic, const T& msg)
{
    if (context_)
    {
        context_->Publish(topic, msg);
    }
}

template <typename T>
inline void Service::Subscribe(const std::string& topic, std::function<void(const T&)> cb)
{
    if (context_)
    {
        try
        {
            auto self = shared_from_this();
            context_->Subscribe(topic, self, cb);
        }
        catch (const std::bad_weak_ptr&)
        {
            throw std::runtime_error("Service::Subscribe: service must be managed by shared_ptr");
        }
    }
}

}  // namespace middleware
