#include <iostream>

int main()
{
  int n = 1;
  if (n > 0) {
    --n;
  } else if (n < 0) {
    ++n;
  } else {
    n = 100;
  }
  std::cout << n << std::endl;  // 0

  int a = 1;
  int b = 2;

  if ((a > 0 && b < 3) || a != b) {
    std::cout << a << std::endl;
  } else {
    std::cout << b << std::endl;
  }

  int max_value = (a > b) ? a : b;
  std::cout << max_value << std::endl;

  enum class State { Idle, Run, Stop };
  auto s = State::Idle;
  switch (s) {
    case State::Idle: {
      if (max_value == 2) {
        s = State::Run;
      }
      break;
    }
    case State::Run: {
      std::cout << "run" << std::endl;
      break;
    }
    default:
      std::cout << "default" << std::endl;
  }

  int x = 1;
  switch (x) {
    case 1:
    case 2:
      std::cout << "x= " << x << std::endl;
      break;
  }

  return 0;
}
