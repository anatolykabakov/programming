#pragma once
#include <string>
#include <iostream>

#include "IPlayer.h"

class User : public IPlayer {
public:
  User(const std::string& name) : name_{name} {}

  void move(Board& board);

  std::string name() { return name_; }

  cell_state cell_type() { return cell_state::cross; }

protected:
  virtual int get_cell_num();

private:
  std::string name_;
};
