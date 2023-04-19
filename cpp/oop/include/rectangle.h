#pragma once
#include "figure.h"
#include <iostream>

namespace figures {
class Rectangle : public Figure {
private:
  double width, height;

public:
  Rectangle(double w, double h) : width(w), height(h) {}
  double getSquare() override { return width * height; }
  double getPerimeter() override { return width * 2 + height * 2; }
  void showFigureType() override { std::cout << "Rectangle" << std::endl; }
};
}  // namespace figures
