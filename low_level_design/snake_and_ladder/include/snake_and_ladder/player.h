#pragma once

#include <random>
#include "snake_and_ladder/board.h"

/**
 * @brief
 * 1. Player solves the task of storing information and moves the ship on the board.
 * 2. The player can ask the board what position to move to if the player hits a snake head,
 * the start of a ladder or goes out of bounds.
 */
class Player {
public:
  Player(int id) : id_{id} {}
  /**
   * @brief Chip position on the board.
   *
   * @return int
   */
  int position() const { return pos_; }
  /**
   * @brief Roll a dice.
   *
   * @return int
   */
  virtual int roll_dice() { return rand() % 6 + 1; }
  /**
   * @brief Move chip by N steps.
   *
   * @param board Board
   * @param steps Number of steps which need move the chip on the board.
   */
  void move(Board& board, int steps);
  /**
   * @brief Returns player id.
   *
   * @return int
   */
  int id() const { return id_; }

private:
  int id_{-1};
  int pos_{0};
};
