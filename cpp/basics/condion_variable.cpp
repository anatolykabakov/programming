#include <iostream>
#include <condition_variable>
#include <thread>
#include <queue>
#include <functional>

void waiter_signaler()
{
  std::mutex m;
  bool ready{false};
  std::condition_variable cv;

  auto waiter = [&]() {
    std::unique_lock<std::mutex> lk(m);
    std::cout << "[waiter] wait..." << std::endl;
    cv.wait(lk, [&] { return ready; });       // unlock lk
    std::cout << "[waiter] go" << std::endl;  // lock lk
  };

  auto signal = [&]() {
    {
      std::lock_guard<std::mutex> lk(m);
      ready = true;
    }
    std::cout << "[signal] notify one" << std::endl;
    cv.notify_one();
  };

  std::thread t1(waiter);
  std::thread t2(signal);
  t1.join();
  t2.join();
}

class SingleThreadExecutor {
public:
  SingleThreadExecutor() { t_ = std::thread(&SingleThreadExecutor::loop_, this); }

  void push(std::function<void()> fn)
  {
    std::unique_lock<std::mutex> lk(m_);
    q_.push(std::move(fn));
    cv_.notify_one();
  }

  ~SingleThreadExecutor()
  {
    {
      std::unique_lock<std::mutex> lk(m_);
      stop_ = true;
      cv_.notify_one();
    }
    if (t_.joinable()) {
      t_.join();
    }
  }

private:
  std::thread t_;
  std::mutex m_;
  std::condition_variable cv_;
  std::queue<std::function<void()>> q_;
  bool stop_{false};

  void loop_()
  {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lk(m_);
        auto pred = [&]() { return stop_ || !q_.empty(); };
        cv_.wait(lk, pred);  // while (!pred()) { cv.wait(lk); }

        if (stop_ && q_.empty()) {
          break;
        }

        task = std::move(q_.front());
        q_.pop();
      }
      task();
    }
  }
};
int main()
{
  waiter_signaler();

  SingleThreadExecutor ste;
  ste.push([]() { std::cout << "hello" << std::endl; });
  ste.push([]() { std::cout << "world!" << std::endl; });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
