#include "../include/middleware.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>

namespace middleware
{

void Service::ScheduleTimer(uint64_t interval_us, std::function<void()> cb)
{
    if (context_)
    {
        try
        {
            auto self = shared_from_this();
            context_->ScheduleTimer(interval_us, self, cb);
        }
        catch (const std::bad_weak_ptr&)
        {
            throw std::runtime_error("Service::ScheduleTimer: Service must be managed by shared_ptr"
            );
        }
    }
}

size_t Service::GetQueueSize() const
{
    if (context_)
    {
        try
        {
            auto self = const_cast<Service*>(this)->shared_from_this();
            return context_->GetQueueSize(self);
        }
        catch (const std::bad_weak_ptr&)
        {
            throw std::runtime_error("Service::GetQueueSize: Service must be managed by shared_ptr"
            );
        }
    }
    return 0;
}

uint64_t Service::Now() const
{
    return context_ ? context_->Now() : 0;
}

void Service::Reset() {}

}  // namespace middleware
