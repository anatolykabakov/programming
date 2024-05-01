#pragma once

#include <vector>

#include "player.h"
#include "board.h"

/**
 * @brief The controller of the game -- managing the interaction of the players and the board, determine the winner.
 * */
class Game {
public:
  /**
   * @brief Construct a new Game object
   *
   * @param players Player.
   */
  Game(const std::vector<Player>& players) : players_{players}, board_(100) {}
  /**
   * @brief Run the game.
   *
   * @return Player
   */
  Player play();

private:
  std::vector<Player> players_;
  Board board_;

private:
  bool is_player_win_(const Player& player) { return player.position() == 100; }
};
