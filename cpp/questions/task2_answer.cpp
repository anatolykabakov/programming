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
 * @brief Этот класс выполняет построение валидной цепочки
   1. Билдер принимает сегменты в любом порядке
   2. Метод build_chain должен построить цепочку из ранее переданных отрезков.
      Если цепочку построить из этих отрезков невозможно, тогда build_chain возвращает nullptr
   3. Цепочка, как и сегменты имеет направление, которое нужно учитывать при валидации
 */
class ChainBuilder {
public:
  ChainBuilder() : chain_(std::make_shared<Chain>()) {}

  void add_segment(Chain::Segment&& segment) { chain_->segments_.push_back(segment); }

  std::shared_ptr<Chain> build_chain(const std::optional<Point> begin_point = std::nullopt)
  {
    // Time: O(N*N)
    for (uint i = 0; i < chain_->segments_.size() - 1; ++i) {
      bool segment_found = false;
      for (uint j = i; j < chain_->segments_.size(); ++j) {
        if (chain_->segments_[i].end_point == chain_->segments_[j].begin_point) {
          std::swap(chain_->segments_[i + 1], chain_->segments_[j]);
          segment_found = true;
        }
      }
      if (!segment_found) {
        return nullptr;
      }
    }
    // Цепочка может начинаться из любой точки.
    // Рассмотрим простой случай --
    // 1. begin_point равен начальной или конечной точке цепочки. Если конечной, то цепочку надо перевернуть.
    // 2. Если начальная точка не содержит значение, то начальная точка выбирается рандомно между начальной
    // и конечной точкой
    // Сложный случай(не реализовано) -- begin_point равен одной из точек в цепочке, тогда
    // 1. Найти begin_point в цепочке.
    // 2. Если begin_point равен конечной точке цепочки, то нужно отбросить сегменты после точки и перевернуть цепочку.
    // 3. Если begin_point равен начачальной точке сегмента, то отбросить сегменты до точки.
    // 4. Если begin_point не указан, то выбирается точка рандомно std::rand() % chain_->segments_.size() - 1
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
    // Если начальная точка не найдена в цепочке, то возвращаем nullptr
    if (!is_end && !is_begin) {
      return nullptr;
    }
    return chain_;
  }

private:
  std::shared_ptr<Chain> chain_;

private:
  void reverse_chain_()
  {
    for (uint i = 0; i < chain_->segments_.size(); ++i) {
      std::swap(chain_->segments_[i].begin_point, chain_->segments_[i].end_point);
    }
    for (uint i = 0; i < chain_->segments_.size() / 2; ++i) {
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
  Примечание: Базовый случай, сегменты переданы последовательно, начальная точка ноль
 */
void testcase1()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{0.0, 0.0}));
  assert((segments[0].end_point == Point{5.0, 0.0}));
  assert((segments[1].begin_point == Point{5.0, 0.0}));
  assert((segments[1].end_point == Point{10.0, 5.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0.0, 0.0), (5.0, 0.0), (10.0, 5.0)
  }

  std::cout << "testcase1 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{6, 0} {10, 0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: nullptr
  Примечание: Цепочка имеет разрыв.
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
  Примечание: Сегменты имеют разные направления
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
  Примечание: Из одного сегмента выходят два. Так не может быть.
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
  Примечание: Сегменты переданы последовательно. Начальная точка равна конечной. Цепочку нужно перевернуть.
 */
void testcase5()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({10.0, 5.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{10.0, 5.0}));
  assert((segments[0].end_point == Point{5.0, 0.0}));
  assert((segments[1].begin_point == Point{5.0, 0.0}));
  assert((segments[1].end_point == Point{0.0, 0.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (10.0, 5.0), (5.0, 0.0), (0.0, 0.0)
  }
  std::cout << "testcase5 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{10, 5} {20, 5}}, {{5, 0} {10, 5}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: {{0, 0} {5, 0}}, {{5, 0} {10, 5}}, {{10, 5} {20, 5}}
  Примечание: Билдер может принимать сегменты в любом порядке. Сегменты переданы не последовательно. Цепочка корректная.
 */
void testcase6()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{10.0, 5.0}, {20.0, 5.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{0.0, 0.0}));
  assert((segments[0].end_point == Point{5.0, 0.0}));
  assert((segments[1].begin_point == Point{5.0, 0.0}));
  assert((segments[1].end_point == Point{10.0, 5.0}));
  assert((segments[2].begin_point == Point{10.0, 5.0}));
  assert((segments[2].end_point == Point{20.0, 5.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0.0, 0.0), (5.0, 0.0), (10.0, 5.0), (20.0, 5.0)
  }
  std::cout << "testcase6 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0, 0} {5, 0}}, {{10, 5} {20, 5}}, {{5, 0} {10, 5}}
  Начальная точка сегмента: {20, 5}
  Выход:
  Цепочка: {{20, 5} {10, 5}}, {{10, 5} {5, 0}}, {{5, 0} {0, 0}}
  Примечание: Билдер может принимать сегменты в любом порядке. Сегменты переданы не последовательно. Цепочка корректная.
 Начальная точка равна конечной. Составить цепочку и перернуть.
 */
void testcase7()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{10.0, 5.0}, {20.0, 5.0}});
  chain_builder.add_segment({{5.0, 0.0}, {10.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({20.0, 5.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{20.0, 5.0}));
  assert((segments[0].end_point == Point{10.0, 5.0}));
  assert((segments[1].begin_point == Point{10.0, 5.0}));
  assert((segments[1].end_point == Point{5.0, 0.0}));
  assert((segments[2].begin_point == Point{5.0, 0.0}));
  assert((segments[2].end_point == Point{0.0, 0.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (20.0, 5.0), (10.0, 5.0), (5.0, 0.0) (0.0, 0.0)
  }
  std::cout << "testcase7 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0.0, 0.0}, {5.0, 0.0}}, {{0.0, 5.0}, {10.0, 10.0}} {{5.0, 0.0}, {0.0, 5.0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: {{0.0, 0.0}, {5.0, 0.0}},  {{5.0, 0.0}, {0.0, 5.0}}, {{0.0, 5.0}, {10.0, 10.0}}
  Примечание: Билдер может принимать сегменты в любом порядке
 */
void testcase8()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{0.0, 5.0}, {10.0, 10.0}});
  chain_builder.add_segment({{5.0, 0.0}, {0.0, 5.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{0.0, 0.0}));
  assert((segments[0].end_point == Point{5.0, 0.0}));
  assert((segments[1].begin_point == Point{5.0, 0.0}));
  assert((segments[1].end_point == Point{0.0, 5.0}));
  assert((segments[2].begin_point == Point{0.0, 5.0}));
  assert((segments[2].end_point == Point{10.0, 10.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0, 0), (5, 0), (0, 5), (10, 10
  }
  std::cout << "testcase8 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0.0, 0.0}, {5.0, 0.0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: {{0.0, 0.0}, {5.0, 0.0}}
  Примечание: Билдер может принимать сегменты в любом порядке
 */
void testcase9()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{0.0, 0.0}));
  assert((segments[0].end_point == Point{5.0, 0.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0.0, 0.0) (5.0, 0.0)
  }
  std::cout << "testcase9 end" << std::endl;
}

/**
 *Вход:
  Сегменты: {{0.0, 0.0}, {5.0, 0.0}}, {{5.0, 0.0}, {0.0, 5.0}}, {{0.0, 5.0}, {0.0, 0.0}}
  Начальная точка сегмента: {0, 0}
  Выход:
  Цепочка: {{0.0, 0.0}, {5.0, 0.0}}, {{5.0, 0.0}, {0.0, 5.0}}, {{0.0, 5.0}, {0.0, 0.0}}
  Примечание: Начальная точка равна и конечной, и начальной цепочки. Будет выбрана конечная точка, цепочка будет
 перевернута.
 */
void testcase10()
{
  ChainBuilder chain_builder;
  chain_builder.add_segment({{0.0, 0.0}, {5.0, 0.0}});
  chain_builder.add_segment({{5.0, 0.0}, {0.0, 5.0}});
  chain_builder.add_segment({{0.0, 5.0}, {0.0, 0.0}});

  auto chain = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain != nullptr);
  const auto& segments = chain->segments();
  assert((segments[0].begin_point == Point{0.0, 0.0}));
  assert((segments[0].end_point == Point{0.0, 5.0}));
  assert((segments[1].begin_point == Point{0.0, 5.0}));
  assert((segments[1].end_point == Point{5.0, 0.0}));
  assert((segments[2].begin_point == Point{5.0, 0.0}));
  assert((segments[2].end_point == Point{0.0, 0.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0, 0), (0, 5), (5, 0), (0, 0)
  }
  std::cout << "testcase10 end" << std::endl;
}

int main()
{
  testcase1();
  testcase2();
  testcase3();
  testcase4();
  testcase5();
  testcase6();
  testcase7();
  testcase8();
  testcase9();
  testcase10();

  return 0;
}
