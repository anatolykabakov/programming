// ref http://scrutator.me/post/2012/04/04/parallel-world-p1.aspx
#include <iostream>
#include <string>
#include <thread>

int main()
{
  auto func = [](const std::string& first, const std::string& second) {
    std::cout << first << second << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  };
  std::thread t1(func, "Hello, ", "threads!");
  t1.join();

  return 0;
}
