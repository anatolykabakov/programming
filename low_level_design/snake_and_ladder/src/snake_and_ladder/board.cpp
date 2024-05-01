#include <vector>

#include "snake_and_ladder/board.h"

Board::Board(int size) : size_{size}
{
  snakes_ = {{95, 1}, {19, 5}, {31, 8}};
  ladders_ = {{3, 45}, {7, 26}, {9, 28}};
}

int Board::on_snake(int pos)
{
  for (const auto& [head, tail] : snakes_) {
    if (head == pos) {
      return tail;
    }
  }
  return pos;
}

int Board::on_ladder(int pos)
{
  for (const auto& [head, tail] : ladders_) {
    if (head == pos) {
      return tail;
    }
  }
  return pos;
}

int Board::out_of_size(int pos)
{
  if (pos > size_) {
    return size_ - (pos - size_);
  }
  return pos;
}
