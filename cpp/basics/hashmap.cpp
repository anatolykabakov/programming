#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>

void print(const std::unordered_map<std::string, int>& m)
{
  for (const auto& [key, value] : m) {
    std::cout << key << " : " << value << std::endl;
  }

  std::cout << "-----------" << std::endl;
}

struct Point {
  int x;
  int y;
};

struct PointHash {
  std::size_t operator()(const Point& p) const { return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1); }
};

struct PointEq {
  bool operator()(const Point& a, const Point& b) const { return a.x == b.x && a.y == b.y; }
};

int main()
{
  std::unordered_map<std::string, int> ages;

  // insert
  ages["anatoly"] = 29;        // create if not exists
  ages.insert({"julia", 22});  // construct in-place with copy
  ages.emplace("carol", 28);   // construct in-container

  // iterate
  print(ages);

  // read
  std::cout << ages["anatoly"] << std::endl;     // 29
  std::cout << ages["anatoly1"] << std::endl;    // создает элемент и 0
  std::cout << ages.at("anatoly") << std::endl;  // 29
  try {
    std::cout << ages.at("anatol2") << std::endl;  // throw _Map_base::at
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << std::endl;
  }
  print(ages);

  // find, count, contains

  if (ages.find("anatoly") != ages.end()) {
    std::cout << ages.at("anatoly") << std::endl;  // 29
  }

  // if (ages.contains("anatoly")) { // C++20
  //     std::cout << ages.at("anatoly") << std::endl; // 29
  // }

  std::cout << ages.count("anatoly") << std::endl;     // 1
  std::cout << ages.count("anatoly222") << std::endl;  // 0

  // update, erase
  ages["julia"] = 31;
  ages.erase("anatoly");
  print(ages);

  for (auto it = ages.begin(); it != ages.end();) {
    if (it->second > 26) {
      it = ages.erase(it);
    } else {
      ++it;
    }
  }

  print(ages);

  // map vs unordered_map
  std::map<std::string, int> ordered;
  ordered["a"] = 1;
  ordered["b"] = 2;
  // a, b -- порядок гарантирован, т.к основан на алгоритме красно-черного дерева
  std::unordered_map<std::string, int> unordered;
  unordered["a"] = 1;
  unordered["b"] = 2;
  // порядок вывода не гарантирован

  // custom key

  std::unordered_map<Point, std::string, PointHash, PointEq> custom;
  custom.insert({Point{1, 2}, "12"});
  custom.insert({Point{3, 2}, "32"});

  for (const auto& [key, value] : custom) {
    std::cout << key.x << " " << key.y << std::endl;
  }

  // live coding

  std::vector<int> v = {1, 2, 3, 4, 2, 3, 4, 2};

  std::unordered_map<int, int> freq;
  for (const auto& el : v) {
    freq[el]++;
  }
  // top K
  std::vector<std::pair<int, int>> tmp(freq.begin(), freq.end());
  std::sort(tmp.begin(), tmp.end(),
            [](const std::pair<int, int>& a, const std::pair<int, int>& b) { return a.second > b.second; });

  std::cout << "------" << std::endl;

  for (int i = 0; i < 2; ++i) {
    std::cout << tmp[i].first << std::endl;
  }

  std::unordered_map<std::string, int> m;
  m["x"];
  std::cout << m["x"] << std::endl;  // 0
  m.try_emplace("a", 1);
  std::cout << m["a"] << std::endl;  // 1
  m.insert_or_assign("a", 2);
  std::cout << m["a"] << std::endl;  // 2
}
