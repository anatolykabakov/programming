#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace tinyware {

class ExecutionManager;

class Node {
protected:
  ExecutionManager* em_ = nullptr;

  template <typename T>
  void publish(const std::string& topic, const T& msg);

  template <typename T>
  void subscribe(const std::string& topic, std::function<void(const T&)> cb);

  void scheduleTimer(uint64_t interval_us, std::function<void()> cb);

  size_t getQueueSize() const;

  uint64_t now() const;

  virtual void configure() = 0;

  virtual void reset();

  friend class ExecutionManager;
};

class ExecutionManager {
public:
  enum class Mode { RealTime, Simulated };

  ExecutionManager(Mode mode, const std::vector<Node*>& nodes) : mode_(mode), sim_time_us_(0)
  {
    for (Node* n : nodes) {
      contexts_.emplace(std::piecewise_construct, std::forward_as_tuple(n), std::forward_as_tuple());
      n->em_ = this;
    }
    for (Node* n : nodes) {
      n->configure();
    }
    // if (mode_ == Mode::RealTime) {
    //     for (auto& kv : contexts_) {
    //         Node* n = kv.first;
    //         Context& ctx = kv.second;
    //         ctx.running = true;
    //         ctx.thread = std::thread([this, n] { runNode(n); });
    //     }
    // }
  }

  bool stop(Node* n)
  {
    if (mode_ != Mode::RealTime) {
      return false;
    }

    if (contexts_.count(n)) {
      auto& ctx = contexts_.at(n);
      if (ctx.running) {
        ctx.running = false;
      }
      if (ctx.thread.joinable()) {
        ctx.thread.join();
        return true;
      }
    }

    return false;
  }

  bool start(Node* n)
  {
    if (mode_ != Mode::RealTime) {
      return false;
    }

    if (contexts_.count(n)) {
      auto& ctx = contexts_.at(n);
      if (!ctx.running) {
        n->reset();
        ctx.running = true;
        ctx.thread = std::thread([this, n] { runNode(n); });
        return true;
      }
    }
    return false;
  }

  bool isRunning(Node* n)
  {
    if (contexts_.count(n)) {
      return contexts_.at(n).running;
    }
    return false;
  }

  void setTime(uint64_t t_us)
  {
    if (mode_ == Mode::Simulated) {
      sim_time_us_ = t_us;
    }
  }

  void step()
  {
    bool work_exists;
    do {
      work_exists = false;
      for (auto& kv : contexts_) {
        if (processMessages(kv.first)) {
          work_exists = true;
        }
      }
    } while (work_exists);

    uint64_t t = now();
    for (auto& kv : contexts_) {
      processTimers(kv.first, t);
    }
  }

  template <typename T>
  void publish(const std::string& topic, const T& msg)
  {
    std::lock_guard<std::mutex> lk(sub_mtx_);
    auto it = subscriptions_.find(topic);
    if (it == subscriptions_.end()) {
      return;
    }
    for (const auto& sub : it->second) {
      auto& ctx = contexts_[sub.node];
      std::lock_guard<std::mutex> ql(ctx.q_mtx);
      ctx.queue.push([cb = sub.cb, data = msg]() { cb(&data); });
    }
  }

  template <typename T>
  void subscribe(const std::string& topic, Node* node, std::function<void(const T&)> cb)
  {
    std::lock_guard<std::mutex> lk(sub_mtx_);
    subscriptions_[topic].push_back({node, [cb](const void* data) { cb(*static_cast<const T*>(data)); }});
  }

  void scheduleTimer(uint64_t interval_us, Node* node, std::function<void()> cb)
  {
    auto& ctx = contexts_[node];
    std::lock_guard<std::mutex> tl(ctx.t_mtx);
    ctx.timers.push_back({interval_us, now() + interval_us, cb});
  }

  size_t getQueueSize(Node* node) const
  {
    auto it = contexts_.find(node);
    if (it == contexts_.end()) {
      throw std::runtime_error("Tried to get queue size for non-existent node");
    }
    const auto& ctx = it->second;
    std::lock_guard<std::mutex> ql(ctx.q_mtx);
    return ctx.queue.size();
  }

  uint64_t now() const
  {
    if (mode_ == Mode::RealTime) {
      return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
    }
    return sim_time_us_;
  }

private:
  struct Subscription {
    Node* node;
    std::function<void(const void*)> cb;
  };
  struct Timer {
    uint64_t interval, next;
    std::function<void()> cb;
  };
  struct Context {
    std::queue<std::function<void()>> queue;
    mutable std::mutex q_mtx;
    std::vector<Timer> timers;
    mutable std::mutex t_mtx;
    std::thread thread;
    std::atomic<bool> running{false};
  };

  bool processMessages(Node* node)
  {
    auto& ctx = contexts_[node];
    std::queue<std::function<void()>> qcopy;
    {
      std::lock_guard<std::mutex> ql(ctx.q_mtx);
      if (ctx.queue.empty()) {
        return false;
      }
      std::swap(qcopy, ctx.queue);
    }
    while (!qcopy.empty()) {
      qcopy.front()();
      qcopy.pop();
    }
    return true;
  }

  void processTimers(Node* node, uint64_t t)
  {
    auto& ctx = contexts_[node];
    std::lock_guard<std::mutex> tl(ctx.t_mtx);
    for (auto& tm : ctx.timers) {
      if (t >= tm.next) {
        tm.cb();
        tm.next += tm.interval;
      }
    }
  }

  void runNode(Node* node)
  {
    sched_param sch_params;
    sch_params.sched_priority = 98;
    auto thread_id = std::this_thread::get_id();
    // the following cast is only valid on Linux and under pthreads
    // TODO: need to check for this explicitly
    auto native_handle = *reinterpret_cast<std::thread::native_handle_type*>(&thread_id);
    pthread_setschedparam(native_handle, SCHED_RR, &sch_params);

    auto& ctx = contexts_[node];

    // reset timers in case the node is being restarted
    const auto curr_time = now();
    for (auto& timer : ctx.timers) {
      timer.next = curr_time + timer.interval;
    }

    ctx.running = true;
    while (ctx.running) {
      processMessages(node);
      processTimers(node, now());
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  Mode mode_;
  uint64_t sim_time_us_;
  std::map<Node*, Context> contexts_;
  std::map<std::string, std::vector<Subscription>> subscriptions_;
  std::mutex sub_mtx_;
};

template <typename T>
inline void Node::publish(const std::string& topic, const T& msg)
{
  em_->publish(topic, msg);
}

template <typename T>
inline void Node::subscribe(const std::string& topic, std::function<void(const T&)> cb)
{
  em_->subscribe(topic, this, cb);
}

inline void Node::scheduleTimer(uint64_t interval_us, std::function<void()> cb)
{
  em_->scheduleTimer(interval_us, this, cb);
}

inline size_t Node::getQueueSize() const { return em_->getQueueSize(const_cast<Node*>(this)); }

inline uint64_t Node::now() const { return em_->now(); }

inline void Node::reset() {}

}  // namespace tinyware
