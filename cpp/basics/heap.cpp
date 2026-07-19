#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>

struct Task {
  std::string name;
  int priority{0};

  bool operator<(const Task& other) const { return priority < other.priority; }

  friend std::ostream& operator<<(std::ostream& os, const Task& task)
  {
    os << task.name << " " << task.priority << std::endl;
    return os;
  }
};

template <typename T, typename Cont, typename Comp>
void print(std::priority_queue<T, Cont, Comp> tasks)
{
  while (!tasks.empty()) {
    std::cout << tasks.top() << " ";
    tasks.pop();
  }
  std::cout << std::endl;
}

std::vector<int> topKGreatest(const std::vector<int>& nums, int k)
{
  std::priority_queue<int> heap;
  for (const auto& num : nums) {
    heap.push(num);  // O(log n)
  }
  std::vector<int> out;
  int count{0};
  while (!heap.empty() && count < k) {
    out.push_back(heap.top());  // O(1)
    heap.pop();
    ++count;
  }
  return out;
}
// {1} -> 1
// {} -> {}
// {1, 1, 1, 2, 2, 2, 3, 4, 5, 5}, 3 -> {1, 2, 5}
std::vector<int> topKFreq(const std::vector<int>& nums, int k)
{
  std::unordered_map<int, int> freq;
  for (const auto& num : nums) {
    if (freq.find(num) == freq.end()) {
      freq.insert(std::make_pair(num, 0));
    }
    ++freq[num];
  }

  auto cmp = [](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second < b.second; };
  std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, decltype(cmp)> max_heap(cmp);  // O(nlogn)
  for (const auto& value : freq) {
    max_heap.push(value);
  }
  std::vector<int> out;
  int count{0};
  while (!max_heap.empty() && count < k) {
    out.push_back(max_heap.top().first);
    max_heap.pop();
    ++count;
  }
  return out;
}

int main()
{
  std::priority_queue<int> pq;  // max heap
  pq.push(3);
  pq.push(1);
  pq.push(4);

  std::cout << pq.top() << std::endl;  // 4
  pq.pop();
  std::cout << pq.top() << std::endl;  // 3

  std::priority_queue<int, std::vector<int>, std::greater<int>> minpq;  // min heap
  minpq.push(3);
  minpq.push(1);
  minpq.push(4);

  std::cout << minpq.top() << std::endl;  // 1

  std::priority_queue<Task> tq;
  tq.push(Task{"one", 1});
  tq.push(Task{"two", 2});
  tq.push(Task{"three", 3});

  std::cout << tq.top().name << std::endl;

  print(tq);

  auto cmp = [](int a, int b) { return a > b; };  // min heap
  std::priority_queue<int, std::vector<int>, decltype(cmp)> pq2(cmp);
  pq2.push(5);
  pq2.push(1);
  pq2.push(3);
  print(pq2);

  auto out = topKGreatest(std::vector<int>{1, 3, 4, 5, 6}, 3);
  for (const auto& num : out) {
    std::cout << num << " ";
  }
  std::cout << std::endl;  // 6, 5, 4

  auto out2 = topKFreq({1, 1, 1, 1, 2, 2, 0, 3, 5, 5, 5}, 3);
  for (const auto& num : out2) {
    std::cout << num << " ";
  }
  std::cout << std::endl;  // 1, 2, 5
}
