#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>

int main()
{
  std::cout << "Main thread id=" << std::this_thread::get_id() << "\n";
  auto asyncDefault =
      std::async([]() { std::cout << "Async default, Threadid=" << std::this_thread::get_id() << "\n"; });

  auto asyncDeffered = std::async(
      std::launch::deferred,
      [](const std::string& str) {
        std::cout << "Async deffer, Threadid=" << std::this_thread::get_id() << "," << str << "\n";
      },
      std::string("end string"));

  auto asyncDeffered2 = std::async(
      std::launch::deferred, []() { std::cout << "Async deffer2, Threadid=" << std::this_thread::get_id() << "\n"; });

  auto trueAsync = std::async(std::launch::async,
                              []() { std::cout << "True async, Threadid=" << std::this_thread::get_id() << "\n"; });
  std::this_thread::sleep_for(std::chrono::seconds(5));
  std::cout << "Sleep ended\n";
  asyncDefault.get();
  asyncDeffered.get();
  trueAsync.get();
}
