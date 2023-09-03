#pragma once
#include <vector>

/**
 * @brief
 * 1. The board solves the task of storing the state of the figures.
 * 2. The board can report which position the player should move to if he got into a position with snake, ladder or out
 * of the board.
 *
 */
class Board {
public:
  Board(int size);
  /**
   * @brief Return position of snake tail, if player moved to position with snake head.
   * @param pos Player position on the board.
   * @return int Position of snake tail.
   */
  virtual int on_snake(int pos);
  /**
   * @brief Return position of ladder head, if player moved to position with ladder tail.
   * @param pos Player position on the board.
   * @return int Position of ladder head.
   */
  virtual int on_ladder(int pos);
  /**
   * @brief Return position by formula: size - (pos - size), if player moved to out the board.
   * @param pos Player position on the board.
   * @return int new position for player.
   */
  virtual int out_of_size(int pos);

private:
  int size_;
  std::vector<std::pair<int, int>> snakes_;
  std::vector<std::pair<int, int>> ladders_;
  // std::vector<std::vector<int>> ships_;
};
