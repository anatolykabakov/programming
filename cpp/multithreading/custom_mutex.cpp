#include <atomic>

class CustomMutex {
public:
  void lock()
  {
    while (m_Locked.test_and_set())
      ;
  }
  void unlock() { m_Locked.clear(); }

private:
  std::atomic_flag m_Locked = ATOMIC_FLAG_INIT;
};

int main()
{
  CustomMutex mutex;
  mutex.lock();
  mutex.unlock();
}
