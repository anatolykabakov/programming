#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <future>

class ThreadPool {
public:
  using Task = std::function<void()>;
  explicit ThreadPool(std::size_t nums_threads) { start_(nums_threads); }
  template <typename T, typename... Args>
  auto enqueue(T task, Args&&... args)
  {
    // Биндим аргументы к функции
    auto func = std::bind(std::forward<T>(task), std::forward<Args>(args)...);
    // Пакуем задачу
    auto wrapper = std::make_shared<std::packaged_task<decltype(task(args...))()>>(func);
    {
      std::unique_lock<std::mutex> lock(event_mutex_);
      // Кладем в очередь
      tasks_.emplace([=]() { (*wrapper)(); });
    }
    // Оповещаем обработчик, что доступна новая задача
    event_.notify_one();
    // Возвращаем отложенный результат.
    return wrapper->get_future();
  }

  ~ThreadPool() { stop_(); }

private:
  /**
   * @brief Функция создает nums_threads обработчиков.
   * Каждый обработчик выполняет таску из очереди tasks_.
   * Если задач в очереди нет, то обработчик ждет сигнала от condition_variable.
   * Обработчик заканчивает свою работу, когда вызывается деструктор и очередь задач пустая.
   *
   * @param nums_threads
   */
  void start_(std::size_t nums_threads)
  {
    for (uint i = 0u; i <= nums_threads; ++i) {
      threads_.emplace_back([=]() {
        while (true) {
          Task task;
          {
            std::unique_lock<std::mutex> lock(event_mutex_);
            event_.wait(lock, [=] { return stop_threads_ || !tasks_.empty(); });
            if (stop_threads_ && tasks_.empty()) {
              break;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          task();
        }
      });
    }
  }

  void stop_() noexcept
  {
    {
      std::unique_lock<std::mutex> lock(event_mutex_);
      stop_threads_ = true;
    }
    event_.notify_all();

    for (auto& thread : threads_) {
      thread.join();
    }
  }

private:
  std::vector<std::thread> threads_;
  std::condition_variable event_;
  std::mutex event_mutex_;
  std::queue<Task> tasks_;
  bool stop_threads_{false};
};

int main()
{
  {
    ThreadPool pool(6);
    auto f1 = pool.enqueue([]() { return 1; });
    auto f2 = pool.enqueue([]() { return 2; });

    std::cout << f1.get() + f2.get() << std::endl;

    auto f3 = pool.enqueue([](int a, int b) { return a + b; }, 2, 3);
    std::cout << f3.get() << std::endl;
  }
  return 0;
}
