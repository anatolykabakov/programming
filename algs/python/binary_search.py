

"""
Бинарный поиск -- алгоритм поиска элемента или индекса элемента в последовательности чисел
Описание: 
1. Принимаем на вход отсортированную последовательность чисел
2. Находим серединный элемент
3. Создаем массив элементов, которые больше серединного, меньше и равны.
4. Если число больше чем серединный элемент, то отбрасываем левую часть
   Если число равно серединному элементу, то число найдено, возмвращаем индекс
   Если число меньше чем серединный элемент, то отбрасываем правую часть
5. повторияем п.4 для новой последовательности

Сложность алгоритма O(log2n)
"""
from math import floor

def bynary_search(arr, number):
    count = 0 # счетчик для оценки сложности
    start = 0
    end = len(arr)
    middle = None
    found = False
    position = -1
    count = 0

    while(found == False and start <= end):
        count += 1
        middle = floor((start + end) / 2)

        if arr[middle] == number:
            found = True
            position = middle
            return position, count
        if (number < arr[middle]):
            end = middle 
        else:
            start = middle

if __name__ == '__main__':
    arr = [1,2,3,4,5,6,7,8,9]
    number = 7
    index, count = bynary_search(arr, number)
    print("index: {}, iterations: {}".format(index, count))