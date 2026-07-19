#include <thread>
#include <iostream>

void work(int id) { std::cout << "id=" << id << std::endl; }

int main()
{
  std::thread t(work, 1);  // copy 1
  t.join();

  std::thread t2([]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << std::this_thread::get_id() << std::endl;

    pthread_setname_np(pthread_self(), "worker1");
  });
  t2.join();

  int x{0};
  auto add = [](int& x) { ++x; };
  std::thread t3(add, std::ref(x));
  t3.join();

  std::cout << x << std::endl;  // 1

  std::thread t4([]() {});
  // auto t5 = t4; // thread(const thread&) = delete;
  auto t5 = std::move(t4);  // ok
  t5.join();
}
