#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sstream>

// Platform-specific includes
#ifdef __ANDROID__
#include <sys/resource.h>  // setpriority
#include <unistd.h>        // getpid, gettid
#include <android/log.h>   // Для логирования в logcat
#else
#include <pthread.h>
#endif

// ========== Unified Logging ==========
namespace microros {
#ifdef __ANDROID__
inline void log_info(const std::string& msg)
{
  __android_log_print(ANDROID_LOG_INFO, "ServicesFramework", "%s", msg.c_str());
}
inline void log_error(const std::string& msg)
{
  __android_log_print(ANDROID_LOG_ERROR, "ServicesFramework", "%s", msg.c_str());
}
#else
inline void log_info(const std::string& msg) { std::cout << msg << std::endl; }
inline void log_error(const std::string& msg) { std::cerr << msg << std::endl; }
#endif
}  // namespace microros

namespace microros {

class ServiceManager;
class Service;

// Умный указатель на Service для удобства
using ServicePtr = std::shared_ptr<Service>;

// ========== Интерфейс для Service - инкапсуляция доступа к ServiceManager ==========
class IServiceContext {
public:
  virtual ~IServiceContext() = default;

  template <typename T>
  void publish(const std::string& topic, const T& msg);

  template <typename T>
  void subscribe(const std::string& topic, const ServicePtr& Service, std::function<void(const T&)> cb) = 0;

  virtual void scheduleTimer(uint64_t interval_us, const ServicePtr& Service, std::function<void()> cb) = 0;
  virtual size_t getQueueSize(const ServicePtr& Service) const = 0;
  virtual uint64_t now() const = 0;

protected:
  // Внутренний метод для реализации publish (шаблонный)
  virtual void publishImpl(const std::string& topic, std::shared_ptr<void> data,
                           std::function<void(const void*)> copier) = 0;
};

// ========== Service - не знает об ServiceManager ==========
class Service : public std::enable_shared_from_this<Service> {
public:
  enum class Priority { Low = 0, Normal = 1, High = 2, Critical = 3 };

protected:
  IServiceContext* context_ = nullptr;    // ✅ Только интерфейс, не весь ServiceManager
  Priority priority_ = Priority::Normal;  // По умолчанию Normal

  template <typename T>
  void publish(const std::string& topic, const T& msg);

  template <typename T>
  void subscribe(const std::string& topic, std::function<void(const T&)> cb);

  void scheduleTimer(uint64_t interval_us, std::function<void()> cb);

  size_t getQueueSize() const;

  uint64_t now() const;

  // Установить приоритет узла (влияет на частоту обработки)
  void setPriority(Priority priority) { priority_ = priority; }

  Priority getPriority() const { return priority_; }

  virtual void configure() = 0;

  virtual void reset();

  // Hook for exception handling
  virtual void onException(const std::exception& e, const std::string& context)
  {
    internal::log_error("Exception in " + context + ": " + e.what());
  }

  friend class ServiceManager;
};

// ========== ServiceManager реализует IServiceContext ==========
class ServiceManager : public IServiceContext {
public:
  enum class Mode { RealTime, Simulated };

  enum class ThreadingMode {
    OneThreadPerService,  // Каждый узел в своем потоке (по умолчанию)
    ThreadPool,           // Фиксированное количество потоков
    SingleThreaded        // Все узлы на одном потоке
  };

  // Конструктор принимает ТОЛЬКО shared_ptr - явная семантика владения
  ServiceManager(Mode mode, const std::vector<ServicePtr>& Services,
                 ThreadingMode threading_mode = ThreadingMode::OneThreadPerService,
                 size_t thread_pool_size = 0)  // 0 = auto (num_cores)
    : mode_(mode), sim_time_us_(0), threading_mode_(threading_mode)
  {
    for (const auto& Service_ptr : Services) {
      if (!Service_ptr) {
        throw std::invalid_argument("ServiceManager: nullptr Service provided");
      }

      // Сохраняем владение
      Services_.push_back(Service_ptr);

      // Используем адрес объекта как уникальный ключ
      Service* key = Service_ptr.get();
      contexts_.emplace(key, Context{Service_ptr});

      // ✅ Устанавливаем интерфейс, а не полный доступ
      Service_ptr->context_ = this;
    }

    for (const auto& Service_ptr : Services_) {
      Service_ptr->configure();
    }

    // Инициализация threading режима
    if (mode_ == Mode::RealTime) {
      if (threading_mode_ == ThreadingMode::ThreadPool) {
        // Если не указано - используем количество ядер CPU
        if (thread_pool_size == 0) {
          thread_pool_size = std::thread::hardware_concurrency();
          if (thread_pool_size == 0)
            thread_pool_size = 4;  // fallback
        }
        thread_pool_size_ = thread_pool_size;
        internal::log_info("ThreadPool mode: using " + std::to_string(thread_pool_size_) + " worker threads");
      } else if (threading_mode_ == ThreadingMode::SingleThreaded) {
        thread_pool_size_ = 1;
        internal::log_info("SingleThreaded mode: using 1 worker thread");
      }
    }
  }

  ~ServiceManager()
  {
    // Останавливаем все узлы и рабочие потоки
    stopAll();
    stopWorkerThreads();
  }

  bool stop(const ServicePtr& Service)
  {
    if (!Service) {
      internal::log_error("Warning: stop() called with nullptr");
      return false;
    }

    if (mode_ != Mode::RealTime) {
      return false;
    }

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      internal::log_error("Warning: stop() called with unregistered Service");
      return false;
    }

    auto& ctx = it->second;
    if (ctx.running) {
      ctx.running = false;
      ctx.cv.notify_one();

      // В ThreadPool/SingleThreaded режиме поток не joinable
      if (threading_mode_ == ThreadingMode::OneThreadPerService) {
        if (ctx.thread.joinable()) {
          ctx.thread.join();
        }
      }
      return true;  // Узел успешно остановлен
    }

    return false;  // Узел уже не запущен
  }

  bool start(const ServicePtr& Service)
  {
    if (!Service) {
      internal::log_error("Warning: start() called with nullptr");
      return false;
    }

    if (mode_ != Mode::RealTime) {
      return false;
    }

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      internal::log_error("Warning: start() called with unregistered Service");
      return false;
    }

    auto& ctx = it->second;
    if (!ctx.running) {
      Service->reset();
      ctx.running = true;

      // Режим: каждый узел в своем потоке
      if (threading_mode_ == ThreadingMode::OneThreadPerService) {
        // Захватываем shared_ptr в лямбде для продления жизни
        ctx.thread = std::thread([this, Service]() { runService(Service); });
      }
      // В ThreadPool/SingleThreaded режиме потоки уже запущены
      // Узел просто помечается как running и обрабатывается worker'ами

      return true;
    }
    return false;
  }

  // Запустить все узлы
  size_t startAll()
  {
    if (mode_ != Mode::RealTime) {
      internal::log_error("Warning: startAll() only works in RealTime mode");
      return 0;
    }

    // Запускаем рабочие потоки для ThreadPool/SingleThreaded режима
    if (threading_mode_ != ThreadingMode::OneThreadPerService) {
      startWorkerThreads();
    }

    size_t started_count = 0;
    for (const auto& Service_ptr : Services_) {
      if (start(Service_ptr)) {
        ++started_count;
      }
    }
    return started_count;
  }

  // Остановить все узлы
  size_t stopAll()
  {
    if (mode_ != Mode::RealTime) {
      return 0;
    }

    size_t stopped_count = 0;
    for (const auto& Service_ptr : Services_) {
      if (stop(Service_ptr)) {
        ++stopped_count;
      }
    }
    return stopped_count;
  }

  bool isRunning(const ServicePtr& Service)
  {
    if (!Service) {
      return false;
    }
    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      return false;
    }
    return it->second.running;
  }

  // Получить количество запущенных узлов
  size_t getRunningCount() const
  {
    size_t count = 0;
    for (const auto& kv : contexts_) {
      if (kv.second.running) {
        ++count;
      }
    }
    return count;
  }

  // Получить общее количество узлов
  size_t getServiceCount() const { return Services_.size(); }

  // Получить статистику узла
  struct ServiceStatistics {
    uint64_t messages_processed;
    uint64_t timers_fired;
    uint64_t exceptions_caught;
    uint64_t total_processing_time_us;
    uint64_t avg_processing_time_us;
  };

  std::optional<ServiceStatistics> getServiceStats(const ServicePtr& Service) const
  {
    if (!Service) {
      return std::nullopt;
    }

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      return std::nullopt;
    }

    const auto& stats = it->second.stats;
    uint64_t msg_count = stats.messages_processed.load();

    return ServiceStatistics{msg_count, stats.timers_fired.load(), stats.exceptions_caught.load(),
                             stats.total_processing_time_us.load(),
                             msg_count > 0 ? stats.total_processing_time_us.load() / msg_count : 0};
  }

  // Вывести статистику всех узлов
  void printStats() const
  {
    internal::log_info("\n=== Service Statistics ===");
    internal::log_info("Total Services: " + std::to_string(Services_.size()));
    internal::log_info("Running Services: " + std::to_string(getRunningCount()));

    size_t total_messages = 0;
    size_t total_timers = 0;

    for (const auto& kv : contexts_) {
      const auto& stats = kv.second.stats;
      uint64_t msg = stats.messages_processed.load();
      uint64_t tim = stats.timers_fired.load();
      uint64_t exc = stats.exceptions_caught.load();

      total_messages += msg;
      total_timers += tim;

      if (msg > 0 || tim > 0) {
        std::ostringstream oss;
        oss << "  Service " << kv.first << ": msg=" << msg << " timers=" << tim;
        if (exc > 0)
          oss << " exceptions=" << exc;
        internal::log_info(oss.str());
      }
    }

    internal::log_info("Total: " + std::to_string(total_messages) + " messages, " + std::to_string(total_timers) +
                       " timers fired");
  }

  void setTime(uint64_t t_us)
  {
    if (mode_ == Mode::Simulated) {
      sim_time_us_ = t_us;
    }
  }

  void step()
  {
    bool work_exists;
    do {
      work_exists = false;
      for (auto& kv : contexts_) {
        if (processMessages(kv.second)) {
          work_exists = true;
        }
      }
    } while (work_exists);

    uint64_t t = now();
    for (auto& kv : contexts_) {
      processTimers(kv.second, t);
    }
  }

  // ========== Реализация IServiceContext ==========

  template <typename T>
  void subscribe(const std::string& topic, const ServicePtr& Service, std::function<void(const T&)> cb) override
  {
    if (!Service) {
      throw std::invalid_argument("subscribe: nullptr Service");
    }

    std::lock_guard<std::mutex> lk(sub_mtx_);
    subscriptions_[topic].push_back(Subscription{Service,  // Сохраняем shared_ptr
                                                 [cb](const void* data) { cb(*static_cast<const T*>(data)); }});
  }

  void scheduleTimer(uint64_t interval_us, const ServicePtr& Service, std::function<void()> cb) override
  {
    if (!Service) {
      throw std::invalid_argument("scheduleTimer: nullptr Service");
    }

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      throw std::runtime_error("scheduleTimer: Service not registered");
    }

    auto& ctx = it->second;
    std::lock_guard<std::mutex> tl(ctx.t_mtx);
    ctx.timers.push_back({interval_us, now() + interval_us, cb});
  }

  size_t getQueueSize(const ServicePtr& Service) const override
  {
    if (!Service) {
      throw std::invalid_argument("getQueueSize: nullptr Service");
    }

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      throw std::runtime_error("getQueueSize: Service not registered");
    }

    const auto& ctx = it->second;
    std::lock_guard<std::mutex> ql(ctx.q_mtx);
    return ctx.queue.size();
  }

  uint64_t now() const override
  {
    if (mode_ == Mode::RealTime) {
      return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
    }
    return sim_time_us_;
  }

protected:
  // Внутренняя реализация publish через интерфейс
  void publishImpl(const std::string& topic, std::shared_ptr<void> data_ptr,
                   std::function<void(const void*)> deliver) override
  {
    // LOCK ORDER: 1. sub_mtx_ -> 2. q_mtx (per-context)
    std::lock_guard<std::mutex> lk(sub_mtx_);
    auto it = subscriptions_.find(topic);
    if (it == subscriptions_.end()) {
      return;
    }

    for (const auto& sub : it->second) {
      Service* key = sub.Service_ptr.get();
      auto ctx_it = contexts_.find(key);
      if (ctx_it == contexts_.end()) {
        continue;  // Service был удален
      }

      auto& ctx = ctx_it->second;
      {
        std::lock_guard<std::mutex> ql(ctx.q_mtx);
        // Захватываем данные и колбэк (deliver не нужен - просто type safety check)
        ctx.queue.push([cb = sub.cb, data_ptr]() { cb(data_ptr.get()); });
      }

      if (mode_ == Mode::RealTime) {
        ctx.cv.notify_one();

        // Также уведомляем worker'ы в ThreadPool режиме
        if (threading_mode_ != ThreadingMode::OneThreadPerService) {
          work_available_cv_.notify_one();
        }
      }
    }
  }

private:
  struct Subscription {
    ServicePtr Service_ptr;  // shared_ptr вместо сырого указателя
    std::function<void(const void*)> cb;
  };

  struct Timer {
    uint64_t interval, next;
    std::function<void()> cb;
  };

  struct ServiceStats {
    std::atomic<uint64_t> messages_processed{0};
    std::atomic<uint64_t> timers_fired{0};
    std::atomic<uint64_t> exceptions_caught{0};
    std::atomic<uint64_t> total_processing_time_us{0};
  };

  struct Context {
    ServicePtr Service_ptr;  // Владение узлом
    std::queue<std::function<void()>> queue;
    mutable std::mutex q_mtx;
    std::condition_variable cv;
    std::vector<Timer> timers;
    mutable std::mutex t_mtx;
    std::thread thread;
    std::atomic<bool> running{false};

    // Для ThreadPool: мутекс для эксклюзивной обработки узла
    std::mutex processing_mtx;

    // Метрики производительности
    ServiceStats stats;
  };

  bool processMessages(Context& ctx)
  {
    std::queue<std::function<void()>> qcopy;
    {
      std::lock_guard<std::mutex> ql(ctx.q_mtx);
      if (ctx.queue.empty()) {
        return false;
      }
      std::swap(qcopy, ctx.queue);
    }

    size_t msg_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (!qcopy.empty()) {
      try {
        qcopy.front()();
        ++msg_count;
      } catch (const std::exception& e) {
        ctx.stats.exceptions_caught++;
        if (ctx.Service_ptr) {
          ctx.Service_ptr->onException(e, "message callback");
        }
      } catch (...) {
        ctx.stats.exceptions_caught++;
        internal::log_error("Unknown exception in message callback");
      }
      qcopy.pop();
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    ctx.stats.messages_processed += msg_count;
    ctx.stats.total_processing_time_us += duration_us;

    return true;
  }

  void processTimers(Context& ctx, uint64_t t)
  {
    std::lock_guard<std::mutex> tl(ctx.t_mtx);

    for (auto& tm : ctx.timers) {
      // Обрабатываем все пропущенные события
      while (t >= tm.next) {
        try {
          tm.cb();
          ctx.stats.timers_fired++;
        } catch (const std::exception& e) {
          ctx.stats.exceptions_caught++;
          if (ctx.Service_ptr) {
            ctx.Service_ptr->onException(e, "timer callback");
          }
        } catch (...) {
          ctx.stats.exceptions_caught++;
          internal::log_error("Unknown exception in timer callback");
        }
        tm.next += tm.interval;
      }
    }
  }

  std::chrono::microseconds getNextTimerDelay(const Context& ctx)
  {
    std::lock_guard<std::mutex> tl(ctx.t_mtx);

    if (ctx.timers.empty()) {
      return std::chrono::milliseconds(1);
    }

    uint64_t current_time = now();
    uint64_t next_timer_us = UINT64_MAX;

    for (const auto& tm : ctx.timers) {
      if (tm.next < next_timer_us) {
        next_timer_us = tm.next;
      }
    }

    if (next_timer_us <= current_time) {
      return std::chrono::microseconds(0);
    }

    uint64_t delay_us = next_timer_us - current_time;
    return std::chrono::microseconds(std::min(delay_us, uint64_t(1000)));
  }

  void runService(const ServicePtr& Service)
  {
    // Platform-specific thread priority
#ifdef __ANDROID__
    // Android: setpriority (nice values) - не требует root
    // Range: -20 (highest) to 19 (lowest)
    int nice_value = -10;  // Высокий приоритет
    if (setpriority(PRIO_PROCESS, 0, nice_value) != 0) {
      internal::log_error("Warning: Failed to set thread priority (errno=" + std::to_string(errno) + ")");
    }
#else
    // Linux/Desktop: SCHED_RR (требует CAP_SYS_NICE)
    sched_param sch_params;
    sch_params.sched_priority = 98;

    int result = pthread_setschedparam(pthread_self(), SCHED_RR, &sch_params);
    if (result != 0) {
      internal::log_error("Warning: Failed to set real-time priority (error " + std::to_string(result) + ")");
    }
#endif

    Service* key = Service.get();
    auto it = contexts_.find(key);
    if (it == contexts_.end()) {
      return;  // Service был удален
    }

    auto& ctx = it->second;

    // Сброс таймеров при перезапуске
    {
      std::lock_guard<std::mutex> tl(ctx.t_mtx);
      const auto curr_time = now();
      for (auto& timer : ctx.timers) {
        timer.next = curr_time + timer.interval;
      }
    }

    ctx.running = true;

    while (ctx.running) {
      processMessages(ctx);
      processTimers(ctx, now());

      auto timeout = getNextTimerDelay(ctx);

      std::unique_lock<std::mutex> lk(ctx.q_mtx);
      ctx.cv.wait_for(lk, timeout, [&ctx]() { return !ctx.queue.empty() || !ctx.running; });
    }
  }

  // ========== Thread Pool Support ==========

  void startWorkerThreads()
  {
    if (worker_threads_running_) {
      return;  // Уже запущены
    }

    worker_threads_running_ = true;

    for (size_t i = 0; i < thread_pool_size_; ++i) {
      worker_threads_.emplace_back([this]() { workerThreadLoop(); });
    }
  }

  void stopWorkerThreads()
  {
    if (!worker_threads_running_) {
      return;
    }

    worker_threads_running_ = false;

    // Разбудить все worker потоки
    work_available_cv_.notify_all();
    for (auto& kv : contexts_) {
      kv.second.cv.notify_all();
    }

    // Дождаться завершения
    for (auto& thread : worker_threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    worker_threads_.clear();
  }

  void workerThreadLoop()
  {
    // Platform-specific thread priority
#ifdef __ANDROID__
    setpriority(PRIO_PROCESS, 0, -10);  // Высокий приоритет для Android
#else
    sched_param sch_params;
    sch_params.sched_priority = 98;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sch_params);
#endif

    // Счетчик итераций для обработки с учетом приоритета
    uint64_t iteration = 0;

    while (worker_threads_running_) {
      bool work_done = false;
      ++iteration;

      // Динамическое распределение с учетом приоритета
      for (auto& kv : contexts_) {
        auto& ctx = kv.second;

        if (!ctx.running || !ctx.Service_ptr) {
          continue;
        }

        // Обработка с учетом приоритета:
        // Critical: каждую итерацию
        // High: каждую итерацию
        // Normal: каждую 2-ю итерацию
        // Low: каждую 4-ю итерацию
        auto priority = ctx.Service_ptr->getPriority();
        uint64_t skip_factor = 1;
        switch (priority) {
          case Service::Priority::Critical:
            skip_factor = 1;
            break;
          case Service::Priority::High:
            skip_factor = 1;
            break;
          case Service::Priority::Normal:
            skip_factor = 2;
            break;
          case Service::Priority::Low:
            skip_factor = 4;
            break;
        }

        // Пропускаем узел согласно его приоритету
        if (iteration % skip_factor != 0) {
          continue;
        }

        // Пытаемся захватить узел (try_lock вместо lock)
        if (!ctx.processing_mtx.try_lock()) {
          continue;  // Узел занят другим worker'ом, пробуем следующий
        }

        // Захватили узел - обрабатываем
        {
          std::lock_guard<std::mutex> guard(ctx.processing_mtx, std::adopt_lock);

          // Обрабатываем сообщения и таймеры
          if (processMessages(ctx)) {
            work_done = true;
          }
          processTimers(ctx, now());
        }
        // processing_mtx освобождается автоматически
      }

      // Если работы не было - ждем на condition variable
      if (!work_done) {
        std::unique_lock<std::mutex> lk(work_available_mtx_);
        work_available_cv_.wait_for(lk, std::chrono::microseconds(100), [this]() { return !worker_threads_running_; });
      }
    }
  }

  Mode mode_;
  uint64_t sim_time_us_;

  // Threading configuration
  ThreadingMode threading_mode_;
  size_t thread_pool_size_ = 0;
  std::atomic<bool> worker_threads_running_{false};
  std::vector<std::thread> worker_threads_;

  // Для уведомления worker'ов о наличии работы
  std::mutex work_available_mtx_;
  std::condition_variable work_available_cv_;

  // Владение узлами через shared_ptr
  std::vector<ServicePtr> Services_;

  // Контексты: используем сырой указатель как ключ для производительности
  // но Context хранит shared_ptr для владения
  std::map<Service*, Context> contexts_;

  std::map<std::string, std::vector<Subscription>> subscriptions_;
  std::mutex sub_mtx_;
};

// ========== Реализация IServiceContext::publish (шаблонный метод) ==========

template <typename T>
inline void IServiceContext::publish(const std::string& topic, const T& msg)
{
  auto data_ptr = std::make_shared<T>(msg);
  auto deliver = [](const void* ptr) {
    // Просто для type safety проверки
    const T* typed = static_cast<const T*>(ptr);
    (void)typed;
  };
  publishImpl(topic, data_ptr, deliver);
}

// ========== Реализация методов Service ==========

template <typename T>
inline void Service::publish(const std::string& topic, const T& msg)
{
  if (context_) {
    context_->publish(topic, msg);
  }
}

template <typename T>
inline void Service::subscribe(const std::string& topic, std::function<void(const T&)> cb)
{
  if (context_) {
    // Получаем shared_ptr на себя
    try {
      auto self = shared_from_this();
      context_->subscribe(topic, self, cb);
    } catch (const std::bad_weak_ptr&) {
      throw std::runtime_error("Service::subscribe: Service must be managed by shared_ptr");
    }
  }
}

inline void Service::scheduleTimer(uint64_t interval_us, std::function<void()> cb)
{
  if (context_) {
    try {
      auto self = shared_from_this();
      context_->scheduleTimer(interval_us, self, cb);
    } catch (const std::bad_weak_ptr&) {
      throw std::runtime_error("Service::scheduleTimer: Service must be managed by shared_ptr");
    }
  }
}

inline size_t Service::getQueueSize() const
{
  if (context_) {
    try {
      auto self = const_cast<Service*>(this)->shared_from_this();
      return context_->getQueueSize(self);
    } catch (const std::bad_weak_ptr&) {
      throw std::runtime_error("Service::getQueueSize: Service must be managed by shared_ptr");
    }
  }
  return 0;
}

inline uint64_t Service::now() const { return context_ ? context_->now() : 0; }

inline void Service::reset() {}

}  // namespace microros
