#pragma once

#include <random>
#include "snake_and_ladder/board.h"

/**
 * @brief Хранит позицию своей фишки на доске. Может делать ход.
 * Может спросить у доски, попал ли в голову змеи, лестницу или вышел за пределы доски.
 *
 */
class Player {
public:
  Player(int id) : id_{id} {}
  /**
   * @brief Узнать позицию игрока на доске.
   *
   * @return int
   */
  int position() const { return pos_; }
  /**
   * @brief Бросить кубик.
   *
   * @return int
   */
  int roll_dice() { return rand() % 6 + 1; }
  /**
   * @brief Перемещает свою фишку на steps шагов на доске.
   *
   * @param board Игровая доска
   * @param steps Количество шагов на которое нужно переместить фишку
   */
  void move(Board& board, int steps);
  /**
   * @brief Возвращает идентификатор игрока.
   *
   * @return int
   */
  int id() const { return id_; }

private:
  int id_;
  int pos_{0};
};
