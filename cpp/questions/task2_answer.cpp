#include <memory>
#include <iostream>
#include <vector>
#include <optional>
#include <cassert>
#include <bits/stdc++.h>

struct Point {
  double x;
  double y;
};

std::ostream& operator<<(std::ostream& os, const Point& point)
{
  os << "(" << point.x << ", " << point.y << ")";
  return os;
}

bool operator!=(const Point& left, const Point& right) { return left.x != right.x || left.y != right.y; }

bool operator==(const Point& left, const Point& right) { return left.x == right.x && left.y == right.y; }

class Chain {
  friend class ChainBuilder;

public:
  struct Segment {
    Point begin_point;
    Point end_point;
  };

private:
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
      const Point& operator*() const { return first_ ? ptr_->begin_point : ptr_->end_point; }

    private:
      std::vector<Segment>::const_iterator ptr_;
      bool first_;
    };
    Iterator begin_;
    Iterator end_;
    Iterator begin() { return begin_; };
    Iterator end() { return end_; };
  };

public:
  const std::vector<Segment>& segments() const { return segments_; }

  Range points() const { return Range{Range::Iterator{segments_.begin()}, Range::Iterator{segments_.end()}}; }

private:
  std::vector<Segment> segments_;
};

/**
 * @brief Этот класс выполняет задачу валидации цепочки.
 * Цепочка валидна, если:
 * 1. Соседние сегменты цепочки соединены, т.е координаты
 * последней точки первого сегмента и координата первой точки равны.
 * 2. Сегмент имеет одного или ноль соседей в одном направлении
 * Цепочка не валидна, если:
 * 1. Соседние сегменты не имеют общих координат начала и конца
 * 2. Сегмент имеет более одного соседа в одном направлении
 */
class ChainValidator {
public:
  ChainValidator(const std::shared_ptr<Chain>& chain) : chain_(chain) {}
  bool validate_chain()
  {
    bool chain_valid = true;
    chain_valid &= is_segments_connected_();
    return chain_valid;
  }

private:
  std::shared_ptr<Chain> chain_;

private:
  bool is_segments_connected_()
  {
    const auto& segments = chain_->segments();
    if (segments.empty()) {
      return false;
    }
    if (segments.size() < 2) {
      return true;
    }
    for (uint i = 1; i < segments.size(); ++i) {
      bool result = segments[i - 1].end_point != segments[i].begin_point;
      if (segments[i - 1].end_point != segments[i].begin_point) {
        return false;
      }
    }
    return true;
  }
};

/**
 * @brief Этот класс выполняет построение валидной цепочки
   1. Билдер принимает сегменты в любом порядке
   2. Метод build_chain должен построить цепочку из ранее переданных отрезков.
      Если цепочку построить из этих отрезков невозможно, тогда build_chain возвращает nullptr
   3. Цепочка, как и сегменты имеет направление, которое нужно учитывать при валидации
 */
class ChainBuilder {
public:
  ChainBuilder() : chain_(std::make_shared<Chain>()), validator_(std::make_unique<ChainValidator>(chain_)) {}

  void add_segment(Chain::Segment&& segment) { chain_->segments_.push_back(segment); }

  std::shared_ptr<Chain> build_chain(const std::optional<Point> begin_point = std::nullopt)
  {
    // Проверка, что сегменты цепочки соединены корректно
    if (!validator_->validate_chain()) {
      chain_->segments_.clear();
      return nullptr;
    }
    // Цепочка может начинаться с начальной или конечной точки.
    // Если начальная точка не содержит значение, то начальная точка выбирается рандомно между начальной и конечной
    // точкой
    bool is_begin = false;
    bool is_end = false;
    if (begin_point.has_value()) {
      is_begin = begin_point.value() == chain_->segments_.front().begin_point;
      is_end = begin_point.value() == chain_->segments_.back().end_point;
    } else {
      // Если рандомное число нечетное, то вернется true, иначе false
      bool random_bool = static_cast<bool>(std::rand() % 2);
      if (random_bool) {
        is_begin = true;
      } else {
        is_end = true;
      }
    }
    // Если передана начальная точка и она равна конечной точке цепочки, то цепочку надо перевернуть
    if (is_end) {
      reverse_chain_();
    }
    // Если начальная точка невалидна, то возвращаем nullptr
    if (!is_end && !is_begin) {
      return nullptr;
    }
    return chain_;
  }

private:
  std::shared_ptr<Chain> chain_;
  std::unique_ptr<ChainValidator> validator_;

private:
  void reverse_chain_()
  {
    for (uint i = 0; i < chain_->segments_.size() / 2; ++i) {
      std::swap(chain_->segments_[i].begin_point, chain_->segments_[i].end_point);
      std::swap(chain_->segments_[chain_->segments_.size() - 1 - i].begin_point,
                chain_->segments_[chain_->segments_.size() - 1 - i].end_point);
      std::swap(chain_->segments_[i], chain_->segments_[chain_->segments_.size() - 1 - i]);
    }
  }
};

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{5, 0} {10, 5}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: {{0, 0} {5, 0}}, {{5, 0} {10, 5}}
 */
void testcase1()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0.0, 0.0), (5.0, 0.0), (10.0, 5.0)
  }
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{6, 0} {10, 0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: nullptr
 */
void testcase2()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{6.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain == nullptr);
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{10, 5} {5, 0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: nullptr
 */
void testcase3()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{10.0, 5.0}, {5.0, 0.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain == nullptr);
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{5, 0} {10, 5}}, {{5, 0} {10, 7}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: nullptr
 */
void testcase4()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 7.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain == nullptr);
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{5, 0} {10, 5}}
  Начальная точка сегмента: {10, 5}
  Выход:
  Цепочка: {{10, 5} {5, 0}}, {{5, 0} {0, 0}}
 */
void testcase5()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({10.0, 5.0}));

  assert(chain != nullptr);

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (10.0, 5.0), (5.0, 0.0), (0.0, 0.0)
  }
}

int main()
{
  testcase1();
  testcase2();
  testcase3();
  testcase4();
  testcase5();

  return 0;
}
