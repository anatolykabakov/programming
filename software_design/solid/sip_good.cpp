#include <iostream>
#include <vector>
/*
 Задача: необходимо разработать систему продаж.
 Система продаж состоит должна поддерживать функции
  1. Добавить товар в заказ
  2. Вывести информацию о заказе
  3. Оплатить заказ
*/

struct Item {
  std::string name;
  float price;
  int quantity;
};

enum class PaymentStatus { NotPaid, Paid };
/*
  Класс обрабатывает единицы товара
  1. добавляет заказ в список
  2. выводит информацию о заказе
*/
class Order {
public:
  Order() {}

  void add(const Item& item) { items.push_back(item); }

  void print() {
    for (const auto& item : items) {
      std::cout << "name " << item.name << " price " << item.price << " quantity " << item.quantity << std::endl;
    }
  }

  float price() {
    float total_price = 0;
    for (const auto& item : items) {
      total_price += item.price * item.quantity;
    }
    return total_price;
  }

private:
  std::vector<Item> items;
};
/*
  Класс обрабатывает единицы товара
  1. оплачивает заказ
*/
struct Payment {
  void pay(Order order, float money) {
    if (money >= order.price()) {
      std::cout << "order payed!" << std::endl;
    } else {
      std::cout << "not enouth money!" << std::endl;
    }
  }
};

int main() {
  Order order;
  order.add(Item{"One", 200.0f, 2});
  order.add(Item{"Two", 300.0f, 1});
  order.print();
  Payment payment;
  payment.pay(order, 100.0f);
  payment.pay(order, 1000.0f);
  return 0;
}
