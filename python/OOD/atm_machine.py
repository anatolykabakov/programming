#!/usr/bin/env python3

"""
ref: LeetCode 2241. Design an ATM Machine

1. Описание
Необходимо спроектировать и реализовать банкомат.
Банкомат -- это устройство для выдачи и приема денег.

2. Вопросы к системе
- Купюры какого номинала может принимать и выдавать?
- Есть ли приоритет по выдаче купюр?
- Какой интерфейс выдачи купюр?
- Какой интерфейс приема купюр?

3. Требования
- Банкомат может принимать купюры 5 номиналов: 20, 50, 100, 200, 500.
- При выдаче купюр есть приоритет. Сначала выдаются купюры большего номинала.
- В банкомат можно ввести количество денег, которое пользователь хочет снять.
Выходом является количество купюр 5 номиналов.
- В банкомат можно внести купюры 5 номиналов.

4. Сценарии использования

- Для снятия денег необходимо ввести количество денег. Банкомат выдает купюры 5 номиналов с приоритетом.
- Для пополнения необходимо внести купюры 5 номинатов. Банкомат пополняет баланс.

5. Дизайн (Абстракция)

- Введем класс ATM, который будет выполнять функции хранения баланса, снятия и пополнения денег.

6. Тесткейсы

Input
["ATM", "deposit", "withdraw", "deposit", "withdraw", "withdraw"]
[[], [[0,0,1,2,1]], [600], [[0,1,0,1,1]], [600], [550]]
Output
[null, null, [0,0,1,0,1], null, [-1], [0,1,0,0,1]]

Explanation
ATM atm = new ATM();
atm.deposit([0,0,1,2,1]); // Deposits 1 $100 banknote, 2 $200 banknotes,
                          // and 1 $500 banknote.
atm.withdraw(600);        // Returns [0,0,1,0,1]. The machine uses 1 $100 banknote
                          // and 1 $500 banknote. The banknotes left over in the
                          // machine are [0,0,0,2,0].
atm.deposit([0,1,0,1,1]); // Deposits 1 $50, $200, and $500 banknote.
                          // The banknotes in the machine are now [0,1,0,3,1].
atm.withdraw(600);        // Returns [-1]. The machine will try to use a $500 banknote
                          // and then be unable to complete the remaining $100,
                          // so the withdraw request will be rejected.
                          // Since the request is rejected, the number of banknotes
                          // in the machine is not modified.
atm.withdraw(550);        // Returns [0,1,0,0,1]. The machine uses 1 $50 banknote
                          // and 1 $500 banknote.

7. Ограничения

banknotesCount.length == 5
0 <= banknotesCount[i] <= 109
1 <= amount <= 109
At most 5000 calls in total will be made to withdraw and deposit.
At least one call will be made to each function withdraw and deposit.

"""

from typing import List


class ATM:
    def __init__(self):
        # key - nominal, value - amount (1,2,3,4..)
        self.banknotes = {20: 0, 50: 0, 100: 0, 200: 0, 500: 0}
        self.__index_nominal_mapping = [(0, 20), (1, 50), (2, 100), (3, 200), (4, 500)]

    def deposit(self, banknotesCount: List[int]) -> None:
        for index, nominal in self.__index_nominal_mapping:
            self.banknotes[nominal] += banknotesCount[index]

    def withdraw(self, amount: int) -> List[int]:
        # Проверим есть ли нужное количество банкнот для входящей суммы.
        banknotes = [0, 0, 0, 0, 0]
        for index, nominal in list(reversed(self.__index_nominal_mapping)):
            if amount >= nominal:
                # Определим сколько банкнот этого номинала требуется
                how_many_banknotes_needed = amount // nominal
                # Определим сколько банкнот данного номинала можем взять.
                how_many_banknotes_can_we_take = min(
                    self.banknotes[nominal], how_many_banknotes_needed
                )
                # Расчет остаточной суммы
                amount -= how_many_banknotes_can_we_take * nominal

                banknotes[index] = how_many_banknotes_can_we_take
        # Удалось ли набрать нужное количество банкнот для входной суммы
        if amount:
            return [-1]
        # Списываем со счета банкноты
        for index, nominal in self.__index_nominal_mapping:
            self.banknotes[nominal] -= banknotes[index]

        return banknotes


if __name__ == "__main__":
    atm = ATM()
    atm.deposit([0, 0, 1, 2, 1])
    banknotes_count = atm.withdraw(600)
    print(banknotes_count)
