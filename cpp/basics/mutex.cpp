#include <mutex>
#include <thread>
#include <iostream>

struct SafeCounter {
  void add(int v)
  {
    std::unique_lock<std::mutex> lock(m_);
    counter_ += v;
  }

  int get() const
  {
    std::unique_lock<std::mutex> lock(m_);
    return counter_;
  }

private:
  mutable std::mutex m_;  // mutable lock in const method
  int counter_{0};
};

int main()
{
  int counter{0};
  std::mutex m;
  auto add = [&]() {
    for (int i = 0; i < 10000; ++i) {
      std::lock_guard<std::mutex> lock(m);
      // m.lock();
      // throw -> deadlock при ручном захвате m.lock
      ++counter;
      // m.unlock();
    }
  };
  std::thread t1(add);
  std::thread t2(add);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  t1.join();
  t2.join();
  std::cout << counter << std::endl;

  if (m.try_lock()) {
    // do work
    m.unlock();
  } else {
    // dont wait work
  }
}
