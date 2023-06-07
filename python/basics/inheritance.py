#!/usr/bin/env python3
class Person:
    def __init__(self, name):
        self.__name = name  # имя человека

    @property
    def name(self):
        return self.__name

    def display_info(self):
        print(f"Name: {self.__name}")


class Employee(Person):
    def __init__(self, name, company):
        super().__init__(name)
        self.company = company

    def display_info(self):
        super().display_info()
        print(f"Company: {self.company}")

    def work(self):
        print(f"{self.name} works")


if __name__ == "__main__":
    tom = Employee("Tom", "Microsoft")
    tom.display_info()  # Name: Tom
    # Company: Microsoft
