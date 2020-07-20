#pragma once
#include "figure.h"
#include <iostream>

namespace figures {
class Circle : public Figure {
  private:
    double radius;
  public:
    Circle(double radius) {
      this->radius = radius;
    }
    double getSquare() override {
      return radius * radius * 3.14;
    }
    double getPerimeter() override {
        return 2 * 3.14 * radius;
    }
    void showFigureType() override {
        std::cout << "Circle" << std::endl;
    }
};
}