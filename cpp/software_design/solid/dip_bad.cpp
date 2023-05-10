#include <iostream>
#include <memory>
#include <vector>

struct Item {
    std::string name;
    float price;
    int quantity;
};

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

struct IPayment {
    virtual void pay(Order order, float money) = 0;
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

struct SMSAutorizer {
    SMSAutorizer() : is_autorized_{false} {}
    void verify(const std::string &code) {
        std::cout << "SMSAutorizer: Verify code" << std::endl;
        std::cout << "SMSAutorizer: autorized!!" << std::endl;
        is_autorized_ = true;
    }
    bool is_autorizer() { return is_autorized_; }

private:
    bool is_autorized_;
};

struct PaymentDebit : IPayment {
    PaymentDebit(const std::string &security, std::shared_ptr<SMSAutorizer> autorizer)
        : security_(security), autorizer_(autorizer) {}

    void pay(Order order, float money)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        if (!autorizer_->is_autorizer()) {
            std::cout << "not autorized!" << std::endl;
        }
        std::cout << "Pay by cash.. Some logic for debit payment this." << std::endl;
        std::cout << "Verify security code: " << security_ << std::endl;
        enough_money(order, money);
    }

private:
    std::string security_;
    std::shared_ptr<SMSAutorizer> autorizer_;
};

struct PaymentCard : IPayment {
    PaymentCard(const std::string &security, std::shared_ptr<SMSAutorizer> autorizer)
        : security_(security), autorizer_(autorizer) {}

    void pay(Order order, float money)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        if (!autorizer_->is_autorizer()) {
            std::cout << "not autorized!" << std::endl;
        }
        std::cout << "Pay by card.. Some logic for card payment this." << std::endl;
        std::cout << "Verify security code: " << security_ << std::endl;
        enough_money(order, money);
    }

private:
    std::string security_;
    std::shared_ptr<SMSAutorizer> autorizer_;
};

struct PaymentPayPal : IPayment {
    PaymentPayPal(const std::string &email) : email_(email) {}

    void pay(Order order, float money)
        override {  // Для добавления нового типа оплаты не надо менять код в Payment
        std::cout << "Pay by PayPal.. Some logic for PayPal payment this." << std::endl;

        std::cout << "Verify email " << email_
                  << std::endl;  // Violation for the principle of Liscov substitution!!!
                                 // PayPal does not handle security code
        enough_money(order, money);
    }

private:
    std::string email_;
};

int main() {
    Order order;
    order.add(Item{"One", 200.0f, 2});
    order.add(Item{"Two", 300.0f, 1});
    order.print();
    const auto debit_autorizer = std::make_shared<SMSAutorizer>();
    PaymentDebit payment("122345", debit_autorizer);
    debit_autorizer->verify("122345");
    payment.pay(order, 100.0f);
    payment.pay(order, 1000.0f);
    const auto card_autorizer = std::make_shared<SMSAutorizer>();
    PaymentCard paymentcard("12345", card_autorizer);
    card_autorizer->verify("122345");
    paymentcard.pay(order, 1000.0f);
    PaymentPayPal paymentpaypal("my@email.com");
    paymentpaypal.pay(order, 1000.0f);
    return 0;
}
