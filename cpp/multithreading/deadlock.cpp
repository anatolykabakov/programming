#include <chrono>
#include <iostream>
#include <limits>
#include <list>
#include <mutex>
#include <thread>

int main()
{
  std::mutex m1;
  std::mutex m2;
  auto deadlock_foo1 = [&m1, &m2]() {
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "lock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.lock();
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.unlock();
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.unlock();
  };
  auto deadlock_foo2 = [&m1, &m2]() {
    std::cout << "lock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.lock();
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.unlock();
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.unlock();
  };
  // std::thread t1(deadlock_foo1);
  // std::thread t2(deadlock_foo2);
  // t1.join();
  // t2.join();

  auto safe_foo1 = [&m1, &m2]() {
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "lock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.lock();
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.unlock();
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.unlock();
  };
  auto safe_foo2 = [&m1, &m2]() {
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.lock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    m2.lock();
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    m1.unlock();
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
    m2.unlock();
  };

  std::thread t3(safe_foo1);
  std::thread t4(safe_foo2);
  t3.join();
  t4.join();

  auto scoped_lock_foo1 = [&m1, &m2]() {
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    std::scoped_lock lock(m1, m2);
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
  };
  auto scoped_lock_foo2 = [&m1, &m2]() {
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    std::cout << "lock m1 thread " << std::this_thread::get_id() << std::endl;
    std::scoped_lock lock(m1, m2);
    std::cout << "do something thread" << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "unlock m1 thread " << std::this_thread::get_id() << std::endl;
    std::cout << "unlock m2 thread " << std::this_thread::get_id() << std::endl;
  };

  std::thread t5(scoped_lock_foo1);
  std::thread t6(scoped_lock_foo2);
  t5.join();
  t6.join();

  return 0;
}
