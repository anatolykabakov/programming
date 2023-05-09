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

enum class PaymentType { CARD, CASH };
/*
  Класс обрабатывает единицы товара
  1. добавляет заказ в список
  2. выводит информацию о заказе
*/
class Order {
public:
    Order() {}

    void add(const Item &item) { items.push_back(item); }

    void print() {
        for (const auto &item : items) {
            std::cout << "name " << item.name << " price " << item.price << " quantity "
                      << item.quantity << std::endl;
        }
    }

    float price() {
        float total_price = 0;
        for (const auto &item : items) {
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

struct IPayment {
    virtual void pay(Order order, float money, const std::string &security) = 0;
    ~IPayment() = default;

protected:
    bool enough_money(Order order, float money) {
        if (money >= order.price()) {
            std::cout << "order payed!" << std::endl;
        } else {
            std::cout << "not enouth money!" << std::endl;
        }
    }
};

struct PaymentDebit : IPayment {
    void pay(Order order,
             float money,
             const std::string &security)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        std::cout << "Pay by cash.. Some logic for debit payment this." << std::endl;
        std::cout << "Verify security code: " << security << std::endl;
        enough_money(order, money);
    }
};

struct PaymentCard : IPayment {
    void pay(Order order,
             float money,
             const std::string &security)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        std::cout << "Pay by card.. Some logic for card payment this." << std::endl;
        std::cout << "Verify security code: " << security << std::endl;
        enough_money(order, money);
    }
};

struct PaymentPayPal : IPayment {
    void pay(Order order,
             float money,
             const std::string &security)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        std::cout << "Pay by PayPal.. Some logic for PayPal payment this." << std::endl;
        std::cout << "Verify email " << security
                  << std::endl;  // Violation for the principle of Liscov substitution!!!
                                 // PayPal does not handle security code
        enough_money(order, money);
    }
};

int main() {
    Order order;
    order.add(Item{"One", 200.0f, 2});
    order.add(Item{"Two", 300.0f, 1});
    order.print();
    PaymentDebit payment;
    payment.pay(order, 100.0f, "12345");
    payment.pay(order, 1000.0f, "12345");
    PaymentCard paymentcard;
    paymentcard.pay(order, 1000.0f, "12345");
    PaymentPayPal paymentpaypal;
    paymentpaypal.pay(order, 1000.0f, "my@email.com");
    return 0;
}
