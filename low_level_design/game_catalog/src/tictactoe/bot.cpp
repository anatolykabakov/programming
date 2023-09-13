#include "tictactoe/bot.h"
#include <random>
#include <iostream>

void Bot::move(Board& board)
{
  int cell = get_cell_num();
  while (board.cells[cell] != cell_state::empty) {
    cell = get_cell_num();
  }
  std::cout << "Bot enter the cell[0-8]: " << cell << std::endl;
  board.cells[cell] = cell_state::zero;
}

int Bot::get_cell_num() { return std::rand() % 9; }
