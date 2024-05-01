#pragma once
#include "board.h"

#include <string>

struct IPlayer {
  virtual void move(Board& board) = 0;
  virtual std::string name() = 0;
  virtual cell_state cell_type() = 0;
  virtual ~IPlayer() = default;
};
