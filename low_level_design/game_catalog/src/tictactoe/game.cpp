#include "tictactoe/game.h"
#include <iostream>

TicTacToe::TicTacToe(std::vector<std::unique_ptr<IPlayer>> players) : players_{std::move(players)}
{
  board_.cells.resize(9, cell_state::empty);
};

std::unique_ptr<IPlayer> TicTacToe::play()
{
  print_board_();
  while (true) {
    for (auto& p : players_) {
      p->move(board_);
      print_board_();
      if (check_winner_(p)) {
        return std::move(p);
      }
    }
  }
}

bool TicTacToe::check_winner_(const std::unique_ptr<IPlayer>& player)
{
  auto check = [&](int i, int j, int k) -> bool {
    return board_.cells[i] == player->cell_type() && board_.cells[j] == player->cell_type() &&
           board_.cells[k] == player->cell_type();
  };
  bool check_rows = check(0, 1, 2) || check(3, 4, 5) || check(6, 7, 8);
  bool check_cols = check(0, 3, 6) || check(1, 4, 7) || check(2, 5, 8);
  bool check_diagonals = check(0, 4, 8) || check(2, 4, 6);
  if (check_rows || check_cols || check_diagonals) {
    return true;
  }

  return false;
}

void TicTacToe::print_board_()
{
  std::cout << "Board:" << std::endl;

  for (int i = 0; i < board_.cells.size(); ++i) {
    if (board_.cells[i] == cell_state::empty) {
      std::cout << " " << i << " ";
    } else if (board_.cells[i] == cell_state::cross) {
      std::cout << " X ";
    } else if (board_.cells[i] == cell_state::zero) {
      std::cout << " O ";
    }

    if ((i + 1) % 3 == 0) {
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;
}
