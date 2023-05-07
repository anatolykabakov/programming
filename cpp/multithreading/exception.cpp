#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <chrono>
#include <exception>

int main() {
  auto f = []() { throw std::logic_error("oops!"); };

  std::future<void> ans = std::async(f);
  std::future<void> ans2 = std::async(f);
  try {
    ans.get();  // method get throwing exception oops
  } catch (const std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }
  ans2.wait();  // method wait does not throw exception

  auto spPromise = std::make_shared<std::promise<void>>();
  auto waiter = spPromise->get_future();
  auto call = [spPromise]() { spPromise->set_exception(std::make_exception_ptr(std::bad_alloc())); };
  std::thread thread(call);
  try {
    waiter.get();
  } catch (std::exception&) {
    std::cout << "catch exception\n";
  }
  thread.join();
}
