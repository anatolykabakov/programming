#include <iostream>
#include <vector>
/*
 ref https://radioprog.ru/post/1420

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

enum class PaymentType { CARD, CASH };

/*
  Зона ответственности:
  1. добавляет заказ в список
  2. выводит информацию о заказе
  3. оплачивает заказ
*/
class GODObjectOrder {
public:
    GODObjectOrder() {}

    void add(const Item &item) { items.push_back(item); }

    void print() {
        for (const auto &item : items) {
            std::cout << "name " << item.name << " price " << item.price << " quantity "
                      << item.quantity << std::endl;
        }
    }

    void pay(float money, PaymentType type, const std::string &security) {
        float total_price = 0;
        for (const auto &item : items) {
            total_price += item.price * item.quantity;
        }

        std::cout << "Verify security code " << security << std::endl;

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
    order.pay(100.0f, PaymentType::CASH, "12345");
    order.pay(1000.0f, PaymentType::CARD, "12345");
    return 0;
}
