#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/resource.h>
#include <unistd.h>

namespace {
void EnableCoreDump()
{
  rlimit core_limit{};
  core_limit.rlim_cur = RLIM_INFINITY;
  core_limit.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE, &core_limit) != 0) {
    std::cerr << "setrlimit(RLIMIT_CORE) failed: " << std::strerror(errno) << std::endl;
  }
}
}  // namespace

int main()
{
  EnableCoreDump();

  std::cout << "PID: " << getpid() << std::endl;
  std::cout << "Crashing now to generate core dump..." << std::endl;

  volatile int* ptr = nullptr;
  *ptr = 42;

  return EXIT_SUCCESS;
}
