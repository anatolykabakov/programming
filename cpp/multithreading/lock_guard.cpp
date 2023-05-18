#include <chrono>
#include <iostream>
#include <limits>
#include <list>
#include <mutex>
#include <thread>

class WarehouseEmpty {
};

unsigned short c_SpecialItem = std::numeric_limits<unsigned short>::max();

class Warehouse {
  std::list<unsigned short> m_Store;

public:
  void AcceptItem(unsigned short item) { m_Store.push_back(item); }
  unsigned short HandLastItem()
  {
    if (m_Store.empty())
      throw WarehouseEmpty();
    unsigned short item = m_Store.front();
    if (item != c_SpecialItem)
      m_Store.pop_front();
    return item;
  }
};

Warehouse g_FirstWarehouse;
Warehouse g_SecondWarehouse;
std::timed_mutex g_FirstMutex;
std::mutex g_SecondMutex;

int main()
{
  auto suplier = []() {
    for (unsigned short i = 0, j = 0; i < 10 && j < 10;) {
      if (i < 100 && g_FirstMutex.try_lock()) {
        g_FirstWarehouse.AcceptItem(i);
        i++;
        g_FirstMutex.unlock();
      }
      if (j < 100 && g_SecondMutex.try_lock()) {
        g_SecondWarehouse.AcceptItem(j);
        j++;
        g_SecondMutex.unlock();
      }
      std::this_thread::yield();
    }
    std::lock_guard<std::timed_mutex> first_guard(g_FirstMutex);
    std::lock_guard<std::mutex> second_guard(g_SecondMutex);
    // g_FirstMutex.lock();
    // g_SecondMutex.lock();
    g_FirstWarehouse.AcceptItem(c_SpecialItem);
    g_SecondWarehouse.AcceptItem(c_SpecialItem);
    // g_FirstMutex.unlock();
    // g_SecondMutex.unlock();
  };
  auto consumer = []() {
    while (true) {
      // g_FirstMutex.lock();
      unsigned short item = 0;
      try {
        std::lock_guard<std::timed_mutex> first_mutex(g_FirstMutex);
        item = g_FirstWarehouse.HandLastItem();
      } catch (Warehouse) {
        std::cout << "Warehouse is empty!\n";
      }
      // g_FirstMutex.unlock();
      if (item == c_SpecialItem)
        break;
      std::cout << "Got new item: " << item << "!\n";
      std::this_thread::sleep_for(std::chrono::seconds(4));
    }
  };
  auto impatientConsumer = []() {
    while (true) {
      unsigned short item = 0;
      if (g_FirstMutex.try_lock_for(std::chrono::seconds(2))) {
        try {
          std::lock_guard<std::timed_mutex> guard(g_FirstMutex, std::adopt_lock);
          item = g_FirstWarehouse.HandLastItem();
        } catch (const WarehouseEmpty&) {
          std::cout << "Warehouse is empty! I'm mad!!!11\n";
        }
      } else {
        std::cout << "First warehouse always busy!!!\n";
        try {
          std::lock_guard<std::mutex> guard(g_SecondMutex);
          item = g_SecondWarehouse.HandLastItem();
        } catch (const WarehouseEmpty&) {
          std::cout << "2nd warehouse is empty!!!!11\n";
        }
      }
      if (item == c_SpecialItem)
        break;
      std::cout << "At last I got new item: " << item << "!\n";
      std::this_thread::sleep_for(std::chrono::seconds(4));
    }
  };

  std::thread supplierThread(suplier);
  std::thread consumerThread(consumer);
  std::thread impatientConsumerThread(impatientConsumer);

  supplierThread.join();
  consumerThread.join();
  impatientConsumerThread.join();
  return 0;
}
