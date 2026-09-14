#include <iostream>
#include <queue>
#include <deque>
#include <unordered_map>
#include <stack>

void BFS(const std::unordered_map<int, int>& g, int start)
{
  std::queue<int> q;
  q.push(start);

  while (!q.empty()) {
    auto value = q.front();
    std::cout << value << " -> ";

    q.pop();

    if (g.find(value) != g.end()) {
      q.push(g.at(value));
    }
  }
  std::cout << std::endl;
}
/**
 * ((())) -> true
 * ((()) - > false
 * ([()]) - > false
 */
bool bracketCorrectness(std::string brackets)
{
  std::stack<char> s;

  for (const auto& bracket : brackets) {
    if (bracket == '(') {
      s.push(bracket);
    } else if (bracket == ')') {
      if (s.empty())
        return false;
      s.pop();
    } else {
      return false;
    }
  }
  return s.empty();
}

int main()
{
  std::queue<int> q;  // FIFO
  q.push(1);
  q.push(2);
  q.push(3);

  std::cout << q.front() << " " << q.back() << " " << q.size() << std::endl;

  auto copy = q;
  while (!copy.empty()) {
    std::cout << copy.front() << " ";
    copy.pop();
  }
  std::cout << std::endl;

  std::deque<int> d{1, 2, 3};
  d.push_back(4);    // 1,2,3,4
  d.push_front(40);  // 0,1,2,3,4
  d.pop_front();
  d.pop_back();
  for (const auto& num : d) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  std::unordered_map<int, int> graph{{0, 1}, {1, 3}, {3, 5}};
  BFS(graph, 0);  // 0 -> 1 -> 3 -> 5

  std::cout << bracketCorrectness("((()))") << std::endl;
  std::cout << bracketCorrectness("(([)))") << std::endl;
  std::cout << bracketCorrectness("((())") << std::endl;
  std::cout << bracketCorrectness("(()))") << std::endl;
  std::cout << bracketCorrectness("(([]))") << std::endl;
}
