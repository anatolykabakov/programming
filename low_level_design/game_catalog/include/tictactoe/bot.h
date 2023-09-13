#pragma once

#include "IPlayer.h"

class Bot : public IPlayer {
public:
  Bot(){};

  void move(Board& board);

  std::string name() { return "Bot"; }

  cell_state cell_type() { return cell_state::zero; }

protected:
  virtual int get_cell_num();
};
