// Есть функция вывода логов, print_log(), сигнатура функции эквивалентна printf().
// Функция блокирующая, для вывода логов использует некий ресурс (например, UART).
// Написать класс для вывода логов, поддерживающий многопоточную работу.

// 1. Создать очередь для хранения текстовых сообщений
// 2. Брать из очереди сообщение и передавать в функцию функцию pring_log
// 3.

void print_log(const std::string& data) {}

class Logger {
public:
  Logger(uint queue_max_size, uint data_max_size) : queue_max_size_{queue_max_size}, data_max_size_{data_max_size},
  {
    t_ = std::thread(Logger::run);
  }

  bool add_task(const std::string& data)
  {
    std::lock_guard<std::mutex> g{m_};
    if (data.size() > data_max_size_ || pq_.size() > queue_max_size_) {
      return false;
    }
    pq_.add(data);
    notifier_.notify_one();
    return true;
  }

  void run()
  {
    while (true) {
      while (!pq_.empty()) {
        std::string data;
        {
          std::lock_guard<std::mutex> g{m_};
          data = pq_.front();
          pq_.pop_back();
        }
        if (data) {
          print_log(data);
        }
      }
      notifier_.wait();
    }
  }

  ~Logger() {}

private:
  std::mutex m_;
  std::queue<std::string> pq_;
  std::thread t_;
  uint data_max_size_;
  uint queue_max_size_;
  std::unique_lock<std::mutex> lock(mutex);
  std::condition_variable notifier_;
};
