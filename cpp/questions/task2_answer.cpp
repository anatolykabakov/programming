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
  struct Range {
    struct Iterator {
      Iterator(std::vector<Segment>::const_iterator ptr) : ptr_(ptr), first_{true} {}
      Iterator operator++()
      {
        if (first_) {
          first_ = false;
          return *this;
        }
        ++ptr_;
        return *this;
      }
      bool operator!=(const Iterator& other) const { return ptr_ != other.ptr_; }
      const Point& operator*()
      {
        if (first_) {
          return ptr_->begin_point;
        }
        return ptr_->end_point;
      }

    private:
      std::vector<Segment>::const_iterator ptr_;
      bool first_;
    };
    Iterator begin_;
    Iterator end_;
    Iterator begin() { return begin_; };
    Iterator end() { return end_; };
  };

  const std::vector<Segment>& segments() const { return segments_; }

  Range points() const { return Range{Range::Iterator{segments_.begin()}, Range::Iterator{segments_.end()}}; }

private:
  std::vector<Segment> segments_;

private:
};

// 1. Билдер принимает сегменты в любом порядке
// 2. Метод build_chain должен построить цепочку из ранее переданных отрезков.
//    Если цепочку построить из этих отрезков невозможно, тогда build_chain возвращает nullptr
// 3. Цепочка, как и сегменты имеет направление, которое нужно учитывать при валидации
class ChainBuilder {
public:
  ChainBuilder() : chain_(std::make_shared<Chain>()) {}

  void add_segment(Chain::Segment&& segment) { chain_->segments_.push_back(segment); }

  std::shared_ptr<Chain> build_chain(const std::optional<Point> begin_point = std::nullopt)
  {
    if (is_chain_valid_()) {
      return chain_;
    } else {
      chain_->segments_.clear();
      return nullptr;
    }
  }

private:
  std::shared_ptr<Chain> chain_;

private:
  bool is_chain_valid_() { return true; }
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
