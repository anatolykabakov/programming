#pragma once

#include <vector>

#include "player.h"
#include "board.h"

/**
 * Класс Game Отвечает за процесс игры - взаимодейвие доски, игроков, определение победителя.
 * */
class Game {
public:
  Game(const std::vector<Player>& players) : players_{players}, board_(100) {}

  Player play();

private:
  std::vector<Player> players_;
  Board board_;

private:
  bool is_player_win_(const Player& player) { return player.position() == 100; }
};
