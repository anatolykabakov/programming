#include <memory>
#include <iostream>
#include <vector>
#include <optional>
#include <cassert>
#include <bits/stdc++.h>

/**
 * TODO:
 * 1. Проверить и убрать лишние аллокации и копирования точек
 * 2. Посмотреть требования stl к итераторам и дописать итератор, если требуется корректная работа с контейнерами stl
 * 3. Порефакторить код. Код определения начала и конца цепочки явно нужно вынести в отдельный метод
 * 4. Отделить описания от определений.
 * 5. Тесткейсы сделать через gtest
 * 6. Добавить описания классов и методов doxygen.
 *
 */

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
  ChainBuilder() {}

  void add_segment(Chain::Segment&& segment) { segments_.push_back(std::forward<Chain::Segment>(segment)); }

  std::shared_ptr<Chain> build_chain(const std::optional<Point> begin_point = std::nullopt)
  {
    bool segments_connected = connect_segments_();
    if (!segments_connected) {
      return nullptr;
    }
    // Цепочка может начинаться из любой точки.
    // Простой случай
    // 1. begin_point равен начальной или конечной точке цепочки. Если конечной, то цепочку надо перевернуть.
    // 2. Если начальная точка не содержит значение, то начальная точка выбирается рандомно между начальной
    // и конечной точкой рандомно
    // Сложный случай(не реализовано) -- begin_point равен одной из точек в цепочке, тогда
    // 1. Найти begin_point в цепочке.
    // 2. Если begin_point равен конечной точке цепочки, то нужно отбросить сегменты после точки и перевернуть цепочку.
    // 3. Если begin_point равен начачальной точке сегмента, то отбросить сегменты до точки.
    // 4. Если begin_point не указан, то выбирается точка рандомно std::rand() % segments_.size() - 1
    // Примечание: В сложном случае есть неопределенность выбора направления. Пример.
    // Вход:
    // Сегменты: {{0, 0} {5, 0}}, {{5, 0} {10, 0}}, {{10, 0} {15, 0}}
    // Начальная точка сегмента: {5, 0}
    // Выход:
    // Цепочка: {{5, 0} {10, 0}}, {{10, 0} {15, 0}}
    // В таком кейсе ответ может быть {5, 0} {0, 0} или {{5, 0} {10, 0}}, {{10, 0} {15, 0}} в зависимости от реализации.
    // Требуется уточнение хочется ли такое поведение или достаточно простого случая

    bool is_begin = false;
    bool is_end = false;
    if (begin_point.has_value()) {
      is_begin = begin_point.value() == segments_.front().begin_point;
      is_end = begin_point.value() == segments_.back().end_point;
    } else {
      // Если рандомное число нечетное, то вернется true, иначе false
      bool random_bool = static_cast<bool>(std::rand() % 2);
      if (random_bool) {
        is_begin = true;
      } else {
        is_end = true;
      }
    }
    if (is_begin == true && is_end == true) {
      //  Если цепочка замкнута, то выбираем начальную точку
      is_end = false;
    }
    // Если передана начальная точка и она равна конечной точке цепочки, то цепочку надо перевернуть
    if (is_end) {
      reverse_segments_();
    }
    // Если начальная точка не найдена в цепочке, то возвращаем nullptr
    if (!is_end && !is_begin) {
      return nullptr;
    }
    std::shared_ptr<Chain> chain = std::make_shared<Chain>();
    chain->segments_ = std::move(segments_);
    return chain;
  }

private:
  std::vector<Chain::Segment> segments_;

private:
  void reverse_segments_()
  {
    for (uint i = 0; i < segments_.size(); ++i) {
      std::swap(segments_[i].begin_point, segments_[i].end_point);
    }
    for (uint i = 0; i < segments_.size() / 2; ++i) {
      std::swap(segments_[i], segments_[segments_.size() - 1 - i]);
    }
  }

  bool connect_segments_()
  {
    // Time: O(N*N)
    for (uint i = 0; i < segments_.size() - 1; ++i) {
      bool segment_found = false;
      for (uint j = i; j < segments_.size(); ++j) {
        if (segments_[i].end_point == segments_[j].begin_point) {
          std::swap(segments_[i + 1], segments_[j]);
          segment_found = true;
        }
      }
      if (!segment_found) {
        return false;
      }
    }
    return true;
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
  Примечание: Неопределенность. Начальная точка равна и конечной, и начальной точке цепочки. Будет выбрана начальная
 точка.
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
  assert((segments[0].end_point == Point{5.0, 0.0}));
  assert((segments[1].begin_point == Point{5.0, 0.0}));
  assert((segments[1].end_point == Point{0.0, 5.0}));
  assert((segments[2].begin_point == Point{0.0, 5.0}));
  assert((segments[2].end_point == Point{0.0, 0.0}));

  for (const auto& point : chain->points()) {
    std::cout << point << std::endl;  // (0, 0), (5, 0), (0, 5), (0, 0)
  }

  chain_builder.add_segment({{10.0, 0.0}, {51.0, 0.0}});
  chain_builder.add_segment({{51.0, 0.0}, {0.0, 5.0}});
  chain_builder.add_segment({{0.0, 5.0}, {0.0, 0.0}});

  auto chain2 = chain_builder.build_chain(std::optional<Point>({0.0, 0.0}));

  assert(chain2 != nullptr);
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
