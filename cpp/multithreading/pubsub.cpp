// ref https://codereview.stackexchange.com/questions/278528/a-toy-application-of-pub-sub-using-c-for-self-learning
#include <memory>
#include <mutex>
#include <deque>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <thread>
#include <condition_variable>

static int i = 0;

struct TopicBase {
  virtual ~TopicBase() = default;
  virtual std::vector<std::function<void()>> handle() = 0;
};

template <class T>
class Topic : public TopicBase {
public:
  Topic() = default;

  void enqueue(std::shared_ptr<T const> message)
  {
    std::lock_guard<std::mutex> lock(m_);
    messages_.push_back(message);
  }

  std::shared_ptr<T const> dequeue()
  {
    std::lock_guard<std::mutex> lock(m_);
    if (!messages_.empty()) {
      auto item = messages_.front();
      messages_.pop_front();
      return item;
    }
    return nullptr;
  }

  template <typename CallBackT>
  void registerCallback(CallBackT&& callback)
  {
    std::lock_guard<std::mutex> lock(m_);
    callbacks_.push_back(std::forward<CallBackT>(callback));
  }

  std::vector<std::function<void()>> handle() override
  {
    std::vector<std::function<void()>> tasks;
    while (!messages_.empty()) {
      auto message = dequeue();

      for (auto&& callback : callbacks_) {
        tasks.push_back(std::bind(callback, message));
      }
    }
    return tasks;
  }

private:
  std::mutex m_;
  std::deque<std::shared_ptr<T const>> messages_;
  std::vector<std::function<void(std::shared_ptr<T const>)>> callbacks_ = {};
};

class Broker {
public:
  Broker() { t_ = std::thread(&Broker::handleTopics, this); }

  template <typename T>
  void publish(std::string topicName, std::shared_ptr<T const> message)
  {
    std::unique_lock<std::mutex> lock(q_);
    if (topics_.find(topicName) == topics_.end()) {
      topics_.insert(std::make_pair(topicName, std::make_shared<Topic<T const>>()));
    }
    auto topic = get<T>(topicName);
    topic->enqueue(message);
    lock.unlock();
    cv_.notify_one();
  }

  template <class T, typename CallbackT>
  void subscribe(std::string topicName, CallbackT callback)
  {
    std::unique_lock<std::mutex> lock(q_);
    if (topics_.find(topicName) != topics_.end()) {
      auto topic = get<T>(topicName);
      topic->registerCallback(std::forward<CallbackT>(callback));
    }
  }

  ~Broker()
  {
    {
      std::unique_lock<std::mutex> lock(q_);
      stop_ = true;
    }
    cv_.notify_all();

    t_.join();
  }

private:
  std::thread t_;
  std::mutex q_;
  std::unordered_map<std::string, std::shared_ptr<TopicBase>> topics_;
  std::condition_variable cv_;
  bool stop_{false};

  template <typename T>
  std::shared_ptr<Topic<T const>> get(std::string topicName)
  {
    auto base = topics_[topicName];
    return std::dynamic_pointer_cast<Topic<T const>>(base);
  }

  void handleTopics()
  {
    while (!stop_) {
      std::vector<std::function<void()>> tasks;
      {
        std::unique_lock<std::mutex> lock(q_);
        cv_.wait(lock, [=]() { return stop_; });

        for (auto& [_, topic] : topics_) {
          auto topicTasks = topic->handle();
          tasks.insert(tasks.end(), topicTasks.begin(), topicTasks.end());
        }
      }
      for (auto&& task : tasks) {
        task();
      }
    }
  }
};

struct Message {
  Message(std::string text) { data = text; }
  std::string data;
};

struct Message2 {
  Message2(std::string text) { data = text; }
  std::string data;
};

class Subscriber {
public:
  Subscriber(std::shared_ptr<Broker> broker) : broker_(std::move(broker))
  {
    auto callbackA = [&](std::shared_ptr<const Message> message) {
      std::cout << "SUB1 topic A " << message->data << " " << i++ << std::endl;
    };
    auto callbackB = [&](std::shared_ptr<const Message> message) {
      std::cout << "SUB1 topic B " << message->data << " " << i++ << std::endl;
    };
    broker_->subscribe<Message>("/topicA", callbackA);
    broker_->subscribe<Message>("/topicB", callbackB);
  }

private:
  std::shared_ptr<Broker> broker_;
};

class Subscriber2 {
public:
  Subscriber2(std::shared_ptr<Broker> broker) : broker_(std::move(broker))
  {
    using namespace std::placeholders;
    broker_->subscribe<Message2>("/topicC", std::bind(&Subscriber2::cb, this, _1));
  }

private:
  std::shared_ptr<Broker> broker_;

  void cb(std::shared_ptr<const Message2> message)
  {
    std::cout << "SUB2 topicC " << message->data << " " << i++ << std::endl;
  };
};

class Publisher {
public:
  Publisher(std::shared_ptr<Broker> broker) : broker_(std::move(broker))
  {
    auto message = std::make_shared<const Message>("hello AB");
    auto messageC = std::make_shared<const Message2>("hello C");
    for (int i = 0; i < 10; ++i) {
      broker_->publish("/topicA", message);
    }
    for (int i = 0; i < 10; ++i) {
      broker_->publish("/topicB", message);
    }
    for (int i = 0; i < 10; ++i) {
      broker_->publish("/topicC", messageC);
    }
  }

private:
  std::shared_ptr<Broker> broker_;
};

int main()
{
  auto broker = std::make_shared<Broker>();
  auto pub = std::make_unique<Publisher>(broker);
  auto sub = std::make_unique<Subscriber>(broker);
  auto sub2 = std::make_unique<Subscriber2>(broker);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
