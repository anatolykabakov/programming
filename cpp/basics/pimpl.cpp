#include <iostream>

#pragma once
#include <memory>

class Widget {
public:
  Widget();
  ~Widget();                             // обязательно объявить здесь
  Widget(Widget&&) noexcept;             // и явно объявить перемещение:
  Widget& operator=(Widget&&) noexcept;  // объявленный деструктор его подавляет
  Widget(const Widget&) = delete;
  Widget& operator=(const Widget&) = delete;

  void addSample(int v);
  double average() const;

private:
  struct Impl;  // объявлен, не определён
  std::unique_ptr<Impl> impl_;
};

// widget.cpp — тяжёлые заголовки только тут
#include "widget.hpp"
#include <numeric>
#include <vector>

struct Widget::Impl {
  std::vector<int> samples;  // меняй это поле — main.cpp пересобирать не нужно
};

Widget::Widget() : impl_(std::make_unique<Impl>()) {}
Widget::~Widget() = default;  // здесь Impl уже полный тип
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;

void Widget::addSample(int v) { impl_->samples.push_back(v); }

double Widget::average() const
{
  if (impl_->samples.empty())
    return 0.0;
  const long long sum = std::accumulate(impl_->samples.begin(), impl_->samples.end(), 0LL);
  return static_cast<double>(sum) / static_cast<double>(impl_->samples.size());
}

int main() {}
