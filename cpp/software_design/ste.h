#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace replay {

class SingleThreadExecutor {
public:
  struct Task {
    std::function<void()> task;
    std::chrono::steady_clock::time_point time;
    std::chrono::milliseconds interval;
    bool periodic{false};
    std::uint64_t sequence{0};
    bool operator>(const Task& other) const
    {
      if (time != other.time) {
        return time > other.time;
      }
      return sequence > other.sequence;
    }
  };
  SingleThreadExecutor();
  ~SingleThreadExecutor();

  void stop();

  void post(std::function<void()> task, std::chrono::milliseconds interval = std::chrono::milliseconds(0),
            bool periodic = false);

private:
  std::atomic<bool> stop_{false};
  std::thread worker_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::priority_queue<Task, std::vector<Task>, std::greater<Task>> tasks_;
  std::uint64_t next_sequence_{0};

  void run();
};

}  // namespace replay
