#include <iostream>
// Default initialization prior to C++11
class Human {
  Human() : age{0} {}

private:
  unsigned age;
};
// Default initialization on C++11
class Human2 {
public:
  unsigned get_age() { return age; }

private:
  unsigned age{0};
};

int main() {
  Human2 h;
  std::cout << h.get_age() << std::endl;
  return 0;
}
