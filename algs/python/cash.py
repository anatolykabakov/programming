"""
кэширование -- процесс запонимания результатов работы функций для повторного использования и экономии ресурсов

Описание;
Создается таблица результатов вызовов функции. 
Если следующий вызов функции с аргументом x найден в таблице,
то берем значение из таблицы, если не найдено,
то вычисляем значение функции и заносим в таблицу кэша
"""

import time


def cash_func(fn):
    cash = {}

def factorial(n):
    """
    3! = 1 * 2 * 3 = 6
    """
    result = 1
    
    for i in range(1, n+1):
        result *= i
    return result


if __name__ == "__main__":
    "без кэшиования"
    result = 0
    start = time.time()
    values = [1000,2000,3000,4000,5000,6000,1000,2000,3000,4000]
    for x in values:
        result += factorial(x)
    end = time.time() - start
    print("Время без кэширования", end)
    "с кэшированием"
    cash = {}
    result = 0
    start = time.time()
    for x in values:
        if not (x in cash):
            cash[x] = factorial(x)
            print("вычисления")
        else:
            print("кэш")
        result += cash[x]
    end = time.time() - start
    print("Время с кэшированием", end)
