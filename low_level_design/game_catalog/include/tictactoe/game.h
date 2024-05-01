#pragma once
#include "registry.h"
#include "IPlayer.h"
#include "IGame.h"
#include <memory>
#include "player.h"
#include "bot.h"

class TicTacToe : public IGame {
public:
  TicTacToe(std::vector<std::unique_ptr<IPlayer>> players);

  std::unique_ptr<IPlayer> play();

  static IGame* create()
  {
    std::vector<std::unique_ptr<IPlayer>> players;
    players.push_back(std::make_unique<User>("User"));
    players.push_back(std::make_unique<Bot>());
    return new TicTacToe(std::move(players));
  }

private:
  std::vector<std::unique_ptr<IPlayer>> players_;
  Board board_;

private:
  bool check_winner_(const std::unique_ptr<IPlayer>& player);

protected:
  virtual void print_board_();
};

// REGISTER_PLUGIN(TicTacToe, TicTacToe::create);
