#include "snake_and_ladder/player.h"

void Player::move(Board& board, int steps)
{
  pos_ += steps;
  pos_ = board.out_of_size(pos_);
  pos_ = board.on_ladder(pos_);
  pos_ = board.on_snake(pos_);
}
