# Requirements

Task: Need to develop the game of Snake and Ladders

## Functional

Game rules:

1. N players are playing in the game.
2. Each player can dice a roll and move сhip on the board.
3. The dice has values ranging from 1 to 6. The value of the dice means of times
   the chip is moved on the board.
4. If a player moves to a square with a snake head, then the player moves to a
   square with a snake tail.
5. If a player moves to a square with a ladder tail, then the player moves to a
   square with a ladder head.

The board: Size is 10 x 10. Contains 10 snakes and ladders. The head of the
snake lies above the tail. The head of the ladder lies below the tail.

## Non-functional

# Entities & relationships

Entities: Player, Board, Game.

Use case:

1. Создаем доску, змеи и лестницы на доске, фишки игроков на клетке 0.
2. Игрок бросает кубик.
3. Игрок двигает фишку на N значений
4. Если фишка оказалась в начале лесницы, то фишка передвигается в конец
   лестницы
5. Если фишка оказалась в голове змеи, то фишка передвигается в хвост змеи.
6. Ход другого игрока
7. Выигрывает игрок, который первым дошел до 100 позиции на доске.

Доска: Состояние: 100 квадратов, в каждом квадрате может быть фишка игрока,
змея, лестница Поведение: Изменить состояние ячейки, узнать состояние ячейки,

Змея: простая Лестница: простая Фишка: простая

Игрок: Состояние: id Действие: передвинуть фишку

Игра: Состояние: Игроки, Доска Поведение: Добавить игрока, начать игру,
определить победителя

Связи: Игрок может изменить состояние доски(передвинуть фишку). Игра может
узнать состояние доски и сообщить о доп перемещении. Доска знает координаты
фишек, змей и лестниц
