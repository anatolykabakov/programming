#pragma once
namespace ns {
class Person {
  private:
    std::string name;
    int age;
  public:
    Person(std::string name, int age) {
      this->name = name;
      this->age = age;
    }
  void printName();
};
}
