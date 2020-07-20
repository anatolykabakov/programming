#pragma once
#include <iostream>
namespace figures {
class Figure {
  public:
    virtual double getSquare() {
        return 0;
    }
    virtual double getPerimeter() {
        return 0;
    }
    virtual void showFigureType() {
        std::cout << "NULL" << std::endl;
    }
};
}