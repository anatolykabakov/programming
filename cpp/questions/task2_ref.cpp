#include <memory>
#include <iostream>
#include <vector>
#include <optional>

struct Point {
  double x;
  double y;
};

std::ostream& operator<<(std::ostream& os, const Point& point)
{
  os << "(" << point.x << ", " << point.y << ")";
  return os;
}

class Chain {
  friend class ChainBuilder;

public:
  struct Segment {
    Point begin_point;
    Point end_point;
  };

  // 1. Реализовать структуру, которую можно использовать в range-based for (должны быть методы begin(), end())
  // 2. Методы begin(), end() должны возвращать итератор по точкам.
  struct ReturnValue {
    Iterator begin() const;
    Iterator end() const;
  };

  const std::vector<Segment>& segments() const;

  const std::vector<Point> points() const {}

private:
  std::vector<Segment> segments_;
};

// 1. Билдер принимает сегменты в любом порядке
// 2. Метод build_chain должен построить цепочку из ранее переданных отрезков.
//    Если цепочку построить из этих отрезков невозможно, тогда build_chain возвращает nullptr
// 3. Цепочка, как и сегменты имеет направление, которое нужно учитывать при валидации
class ChainBuilder {
public:
  void add_segment(Chain::Segment&& segment) {}
  std::shared_ptr<Chain> build_chain(const std::optional<Point> begin_point = std::nullopt)
  {
    // соединяем сегменты тут
  }
};

int main()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain();

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0.0, 0.0), (5.0, 0.0), (10.0, 5.0)
  }

  return 0;
}
