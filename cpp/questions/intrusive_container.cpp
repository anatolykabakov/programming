#include <cstdlib>
#include <ctime>
#include <iostream>

struct Storable {
  int number;
  Storable *prev, *next;
};

template <class T>
class IntrusiveStorage {
public:
  IntrusiveStorage() : origin(0) {}
  void pushBack(T& data)
  {
    if (origin == 0) {
      origin = &data;
      origin->next = origin;
      origin->prev = origin;
    } else {
      data.next = origin;
      data.prev = origin->prev;
      data.next->prev = data.prev->next = &data;
    }
  }
  T& getLast()
  {
    if (origin == 0) {
    }
    return *(origin->prev);
  }
  void popBack(T& data)
  {
    if (origin == 0) {
    }
    if (origin->next = origin) {
      origin = 0;
    } else {
      T* data = origin->prev;
      data->prev->next = data->next;
      data->next->prev = data->prev;
    }
  }

private:
  T* origin;
};

int main(int argc, char** argv)
{
  srand(time(0));
  IntrusiveStorage<Storable> storage;

  Storable s;
  s.number = 5;
  storage.pushBack(s);
  std::cout << storage.getLast().number << std::endl;

  std::cin.get();
  return 0;
}
