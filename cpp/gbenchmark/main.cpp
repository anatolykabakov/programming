// ref http://mochan.info/c++/cmake/tutorial/2019/03/23/gtest-cmake.html

#include <benchmark/benchmark.h>

static void StringCopy(benchmark::State& state)
{
  std::string x = "hello";
  for (auto _ : state)
    std::string copy(x);
}

BENCHMARK(StringCopy);

BENCHMARK_MAIN();
