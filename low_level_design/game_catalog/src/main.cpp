#include "tictactoe/game.h"
#include <iostream>

int main()
{
  // PluginRegistry<IGame>::add("TicTacToe", TicTacToe::create);

  std::string exit_cmd;
  while (exit_cmd != "y") {
    auto available_games = PluginRegistry<IGame>::keys();
    std::cout << "Available games" << std::endl;
    for (int i = 0; i < available_games.size(); ++i) {
      std::cout << i << " " << available_games[i] << std::endl;
    }
    std::cout << "Enter game id" << std::endl;
    int game_ind;
    std::cin >> game_ind;
    auto game = PluginRegistry<IGame>::create(available_games[game_ind]);
    auto winner = game->play();
    std::cout << winner->name() << " wins" << std::endl;
    std::cout << "do yout want exit? [y/n]" << std::endl;
    std::cin >> exit_cmd;
  }
  return 0;
}
