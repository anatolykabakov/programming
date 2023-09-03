#include "snake_and_ladder/game.h"

Player Game::play()
{
  while (true) {
    for (auto& player : players_) {
      player.move(board_, player.roll_dice());
      if (is_player_win_(player)) {
        return player;
      }
    }
  }
}
