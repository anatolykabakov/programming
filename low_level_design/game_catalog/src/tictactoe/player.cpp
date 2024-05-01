#include "tictactoe/player.h"
#include "tictactoe/board.h"

void User::move(Board& board)
{
  int cell = get_cell_num();
  std::cout << "User enter the cell [0-8]: " << cell << std::endl;

  board.cells[cell] = cell_state::cross;
}

int User::get_cell_num()
{
  int cell;
  std::cin >> cell;
  return cell;
}
