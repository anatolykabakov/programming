#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <iostream>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QMainWindow w;
  QPushButton* button = new QPushButton(&w);
  button->setText("hello world");
  QObject::connect(button, &QPushButton::clicked, [=]() { std::cout << "button pressed!" << std::endl; });
  w.show();
  return app.exec();
}
