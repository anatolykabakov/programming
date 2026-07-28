#include <iostream>
#include <mutex>
#include <thread>

void deadlock()
{
  std::mutex m1;
  std::mutex m2;
  int resource{0};

  auto foo = [&]() {
    std::cout << "[foo] try lock m1" << std::endl;
    m1.lock();
    std::cout << "[foo] lock m1 sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "[foo] try lock m2" << std::endl;
    m2.lock();
    std::cout << "[foo] lock m1, m2" << std::endl;
    ++resource;
    std::cout << "[foo] unlock m2" << std::endl;
    m2.unlock();
    std::cout << "[foo] unlock m1" << std::endl;
    m1.unlock();
  };
  auto bar = [&]() {
    std::cout << "[bar] try lock m2" << std::endl;
    m2.lock();
    std::cout << "[bar] lock m2 sleep" << std::endl;
    ++resource;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "[bar] try lock m1" << std::endl;
    m1.lock();
    std::cout << "[bar] lock m2 m1" << std::endl;
    std::cout << "[foo] unlock m2" << std::endl;
    m1.unlock();
    std::cout << "[foo] unlock m1" << std::endl;
    m2.unlock();
  };
  std::thread t1(foo);
  std::thread t2(bar);
  t1.join();
  t2.join();
}

void goodlock()
{
  std::mutex m1;
  std::mutex m2;
  int resource{0};

  auto foo = [&]() {
    // std::scoped_lock<std::mutex> lk(m1, m2);
    std::cout << "[foo] try lock m1" << std::endl;
    m1.lock();
    std::cout << "[foo] lock m1" << std::endl;
    std::cout << "[foo] try lock m2" << std::endl;
    m2.lock();
    std::cout << "[foo] lock m2" << std::endl;
    std::cout << "[foo] sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ++resource;
    std::cout << "[foo] try unlock m2" << std::endl;
    m2.unlock();
    std::cout << "[foo] unlock m2" << std::endl;
    std::cout << "[foo] try unlock m1" << std::endl;
    m1.unlock();
    std::cout << "[foo] unlock m1" << std::endl;
  };
  auto bar = [&]() {
    // std::scoped_lock<std::mutex> lk(m1, m2);
    std::cout << "[bar] try lock m1" << std::endl;
    m1.lock();
    std::cout << "[bar] lock m1" << std::endl;
    std::cout << "[bar] try lock m2" << std::endl;
    m2.lock();
    std::cout << "[bar] lock m2" << std::endl;
    ++resource;
    std::cout << "[bar] sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "[foo] try unlock m2" << std::endl;
    m2.unlock();
    std::cout << "[foo] unlock m2" << std::endl;
    std::cout << "[foo] try unlock m1" << std::endl;
    m1.unlock();
    std::cout << "[foo] unlock m1" << std::endl;
  };
  std::thread t1(foo);
  std::thread t2(bar);
  t1.join();
  t2.join();
}

void recursive_deadlock()
{
  // std::mutex m1;
  std::recursive_mutex m1;

  auto foo = [&]() {
    std::cout << "[foo] try lock m1" << std::endl;
    // std::lock_guard<std::mutex> lk(m1); // deadlock
    std::lock_guard<std::recursive_mutex> lk(m1);
    std::cout << "[foo] unlock" << std::endl;
  };

  auto bar = [&]() {
    std::cout << "[bar] try lock m1" << std::endl;
    // std::lock_guard<std::mutex> lk(m1);
    std::lock_guard<std::recursive_mutex> lk(m1);
    foo();
    std::cout << "[bar] unlock" << std::endl;
  };

  bar();
}

int main()
{
  // deadlock();
  // goodlock();

  recursive_deadlock();
}
