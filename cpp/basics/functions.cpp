#include <iostream>
#include <queue>
#include <functional>

int foo() { return 42; }

int sum(int a, int b) { return a + b; }

double sum(double a, double b) { return a + b; }

void defaultArgs(int a, int b = 2) { std::cout << a + b << std::endl; }

constexpr int compileTimeSum(const int a, const int b)
{
  return a + b;  // compile time
}

int bar(int a);  // declaration (.h)

int bar(int a)
{  // definition (.cpp)
  return a;
}

int sum(std::initializer_list<int> list)
{
  int total = 0;
  for (auto& e : list) {
    total += e;
  }
  return total;
}

template <typename T>
T fooT(T a, T b)
{
  return a + b;
}

template <typename T>
T variadicTAll(T a)
{
  return a;
}

template <typename... Args>
auto variadicTAll(Args... args)
{
  return (... + args);
}

template <typename... Args>
void printSize(Args... args)
{
  std::cout << sizeof...(Args) << std::endl;
  int sizes[] = {sizeof(Args)...};
  for (const auto& size : sizes) {
    std::cout << size << std::endl;
  }
}

class FuncContainer {
public:
  template <typename F, typename... Args>
  void enqueue(F&& f, Args&&... args)
  {
    tasks_.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));  // cpp17
  }

  auto dequeue()
  {
    auto task = std::move(tasks_.front());
    tasks_.pop();
    return task;
  }

private:
  std::queue<std::function<void()>> tasks_;
};

int main()
{
  std::cout << foo() << std::endl;                   // 42
  std::cout << sum(1, 2) << std::endl;               // 3
  std::cout << sum(1.0, 2.5) << std::endl;           // 3.5
  defaultArgs(1);                                    // 3
  defaultArgs(1, 3);                                 // 4
  std::cout << compileTimeSum(1, 2) << std::endl;    // 3
  std::cout << bar(1) << std::endl;                  // 1
  std::cout << sum({1, 2, 3}) << std::endl;          // 6
  std::cout << fooT(1, 2) << std::endl;              // 3
  std::cout << fooT(1.0, 2.5) << std::endl;          // 3.5
  std::cout << variadicTAll(1, 2) << std::endl;      // 3
  std::cout << variadicTAll(1.0, 2.5) << std::endl;  // 3.5
  printSize(1, 2, 3);                                // 3

  auto log = [](const std::string& message) { std::cout << "Log: " << message << std::endl; };

  log("Hello, world!");  // Log: Hello, world!

  // examples of lambda expressions
  auto f = []() { return 42; };
  std::cout << f() << std::endl;             // 42
  auto g = [](int x) { return x * x * x; };  //  x ^ 3
  std::cout << g(3) << std::endl;            // 27

  // capture
  int n = 42;
  auto withCopyN = [n](int x) { return x + n; };
  std::cout << withCopyN(10) << std::endl;
  ;  // 52

  auto byRef = [&n](int x) { return x + n; };
  std::cout << byRef(10) << std::endl;  // 52
  int M = 10;

  auto copyAllLocalVars = [=](int x) { return x + M + n; };
  std::cout << copyAllLocalVars(10) << std::endl;

  // auto errorF = []() { return M + n; }; // error: ‘M’ and 'n' is not captured
  // std::cout << errorF() << std::endl;

  FuncContainer queue;
  queue.enqueue([]() { std::cout << "foo" << std::endl; });

  queue.enqueue([](int x) { std::cout << 1 + x << std::endl; }, 10);

  queue.dequeue()();
  queue.dequeue()();
  return 0;
}
