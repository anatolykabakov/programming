#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <thread>

template <typename It, typename T>
struct accumulate_block {
  void operator()(It first, It last, T& result) { result = std::accumulate(first, last, result); }
};

template <typename It, typename T>
T parallel_accumulate(It first, It last, T init)
{
  auto length = std::distance(first, last);
  if (!length) {
    return init;
  }
  unsigned long min_per_thread = 25;
  unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
  unsigned long hardware_threads = std::thread::hardware_concurrency();
  std::cout << "hardware threads: " << hardware_threads << std::endl;
  unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
  unsigned long block_size = length / num_threads;

  std::vector<T> results(num_threads);
  std::vector<std::thread> threads(num_threads - 1);

  It block_start = first;
  for (unsigned long i = 0; i < (num_threads - 1); ++i) {
    It block_end = block_start;
    std::advance(block_end, block_size);
    threads[i] = std::thread(accumulate_block<It, T>(), block_start, block_end, std::ref(results[i]));
    block_start = block_end;
  }
  accumulate_block<It, T>()(block_start, last, results[num_threads - 1]);

  std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
  return std::accumulate(results.begin(), results.end(), init);
}

int main()
{
  std::vector<int> v(100000000);
  std::iota(v.begin(), v.end(), 0.0);

  // std::for_each(v.begin(), v.end(), [](int a){ std::cout << a << std::endl; });
  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << std::accumulate(v.begin(), v.end(), 0) << std::endl;
  auto t2 = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;

  t1 = std::chrono::high_resolution_clock::now();
  std::cout << parallel_accumulate(v.begin(), v.end(), 0) << std::endl;
  t2 = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;
  return 0;
}
