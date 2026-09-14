
#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace replay {
class PubSub {
public:
  PubSub() = default;
  ~PubSub() = default;

  template <typename T>
  using Callback = std::function<void(const T&)>;

  template <typename T>
  void publish(const std::string& topic, const T& msg)
  {
    // Type-erased invocation, but only callbacks registered for the same T will be called.
    std::vector<std::function<void(const void*)>> callbacks;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = subscriptions_.find(topic);
      if (it == subscriptions_.end()) {
        return;
      }

      const std::type_index wanted{typeid(T)};
      for (const auto& sub : it->second) {
        if (sub.type == wanted) {
          callbacks.push_back(sub.call);
        }
      }
    }

    // Invoke outside the mutex to avoid deadlocks / callback reentrancy issues.
    for (const auto& cb : callbacks) {
      cb(static_cast<const void*>(&msg));
    }
  }

  template <typename T>
  void Subscribe(const std::string& topic, Callback<T> cb)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[topic].push_back(
        Subscription{.type = std::type_index(typeid(T)),
                     .call = [cb = std::move(cb)](const void* raw) { cb(*static_cast<const T*>(raw)); }});
  }

private:
  struct Subscription {
    std::type_index type{typeid(void)};
    std::function<void(const void*)> call;
  };

  std::mutex mutex_;
  std::unordered_map<std::string, std::vector<Subscription>> subscriptions_;
};
}  // namespace replay
