#include <iostream>
#include <execinfo.h>
#include <unistd.h>
#include <csignal>

void crashHandler()
{
  auto printBacktrace = [](int sig) {
    std::signal(sig, SIG_DFL);
    std::cout << "Crash handler" << std::endl;
    void* frames[64];
    int size = backtrace(frames, 64);
    char** symbols = backtrace_symbols(frames, size);
    for (int i = 0; i < size; ++i) {
      std::cout << symbols[i] << std::endl;
    }
    std::raise(sig);
  };
  std::signal(SIGSEGV, printBacktrace);
  std::signal(SIGFPE, printBacktrace);
  std::signal(SIGABRT, printBacktrace);
  std::signal(SIGILL, printBacktrace);
  std::signal(SIGBUS, printBacktrace);
}

class Bar {
public:
  Bar() { std::cout << "Bar constructor" << std::endl; }
  ~Bar() { std::cout << "Bar destructor" << std::endl; }
  void Run()
  {
    volatile int* a = (int*)(NULL);
    *a = 1;
  }
};

class Foo {
public:
  Foo() { std::cout << "Foo constructor" << std::endl; }

  void Run() { bar_.Run(); }

  ~Foo() { std::cout << "Foo destructor" << std::endl; }

private:
  Bar bar_;
};

int main()
{
  Foo foo;
  crashHandler();
  foo.Run();

  return 0;
}
