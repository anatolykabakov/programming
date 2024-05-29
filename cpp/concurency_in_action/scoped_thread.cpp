#include <thread>
#include <iostream>

class scoped_thread {
public:
  explicit scoped_thread(std::thread t) : t_(std::move(t))
  {
    if (!t_.joinable()) {
      throw std::logic_error("No thread!");
    }
  }

  scoped_thread(const scoped_thread&) = delete;
  scoped_thread& operator=(const scoped_thread&) = delete;

  ~scoped_thread() { t_.join(); }

private:
  std::thread t_;
};

int main()
{
  std::thread t([]() {
    for (int i = 0; i < 10000; ++i) {
      std::cout << i << " ";
    }
  });
  scoped_thread g(std::move(t));
  return 0;
}
