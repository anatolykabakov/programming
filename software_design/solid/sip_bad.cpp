#include <iostream>
#include <vector>
/*
 Задача: необходимо разработать систему продаж.
 Система продаж состоит должна поддерживать функции
  1. Добавить товар в заказ
  2. Вывести информацию о заказе
  3. Оплатить заказ
*/

/*
  Зона ответственности:
  1. хранить информацию о товаре
*/
struct Item {
  std::string name;
  float price;
  int quantity;
};

/*
  Зона ответственности:
  1. добавляет заказ в список
  2. выводит информацию о заказе
  3. оплачивает заказ
*/
class GODObjectOrder {
public:
  GODObjectOrder() {}

  void add(const Item& item) { items.push_back(item); }

  void print() {
    for (const auto& item : items) {
      std::cout << "name " << item.name << " price " << item.price << " quantity " << item.quantity << std::endl;
    }
  }

  void pay(float money) {
    float total_price = 0;
    for (const auto& item : items) {
      total_price += item.price * item.quantity;
    }

    if (money >= total_price) {
      std::cout << "order payed!" << std::endl;
    } else {
      std::cout << "not enouth money!" << std::endl;
    }
  }

private:
  std::vector<Item> items;
};

int main() {
  GODObjectOrder order;
  order.add(Item{"One", 200.0f, 2});
  order.add(Item{"Two", 300.0f, 1});
  order.print();
  order.pay(100.0f);
  order.pay(1000.0f);
  return 0;
}
