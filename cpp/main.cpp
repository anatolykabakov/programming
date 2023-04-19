#include <iostream>
#include "oop/include/person.h"
#include "oop/include/circle.h"
#include "oop/include/rectangle.h"
#include "oop/include/rectangle.h"

int main() {
  ns::Person p = ns::Person("Tom", 22);
  p.printName();
  figures::Figure* circle = new figures::Circle(0.5);
  figures::Figure* rectangle = new figures::Rectangle(2, 4);
  circle->showFigureType();
  std::cout << "Perimeter: " << circle->getPerimeter() << " Square: " << circle->getSquare() << std::endl;
  std::cout << "Perimeter: " << rectangle->getPerimeter() << " Square: " << rectangle->getSquare() << std::endl;
  return 0;
}
