#include <thread>
#include <iostream>
#include <mutex>

int main()
{
  int count = 0;
  std::mutex m;
  auto race_condition_foo = [&] {
    for (int i = 0; i < 1000'000; ++i) {
      count++;
    }
  };
  std::thread t1(race_condition_foo);
  std::thread t2(race_condition_foo);

  t1.join();
  t2.join();

  std::cout << count << std::endl;
  count = 0;
  auto safe_foo = [&] {
    for (int i = 0; i < 1000'000; ++i) {
      m.lock();
      count++;
      m.unlock();
    }
  };

  std::thread t3(safe_foo);
  std::thread t4(safe_foo);

  t3.join();
  t4.join();

  std::cout << count << std::endl;

  return 0;
}
