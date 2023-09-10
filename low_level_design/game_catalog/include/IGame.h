#pragma once
#include <memory>
#include "tictactoe/IPlayer.h"

struct IGame {
  virtual std::unique_ptr<IPlayer> play() = 0;
  virtual ~IGame() = default;
};
