#include "snake_and_ladder/game.h"

#include <iostream>

int main()
{
  std::vector<Player> players = {Player{0}, Player{1}};
  Game snake_and_ladder(players);
  auto winner = snake_and_ladder.play();
  std::cout << "Player with id " << winner.id() << " wins" << std::endl;
  return 0;
}
