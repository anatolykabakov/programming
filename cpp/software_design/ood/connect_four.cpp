/**
 * @file connect_four.cpp
 * @author Anatoly Kabakov (anatoly.kabakov@mail.ru)
 * @brief Connect Four is a popular ConnectFourGame played on a 7x6 grid.
 * Two players take turns dropping colored discs into the grid.
 * The first player to get four discs in a row (vertically, horizontally or diagonally)
 * wins.
 * @version 0.1
 * @date 2023-05-13
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <exception>
#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

enum class CellState { EMPTY, YELLOW, RED };

/**
 * @brief We will need a Grid class to maintain the state of the 2-D board
 * The board cell can be empty, yellow (occupied by Player 1) or red (occupied by Player
 *
 */
class Grid {
public:
  Grid(int cols, int rows) : cols_(cols), rows_(rows)
  {
    grid_.resize(rows, std::vector<CellState>(cols, CellState::EMPTY));
  }

  void reset()
  {
    grid_.clear();
    grid_.resize(rows_, std::vector<CellState>(cols_, CellState::EMPTY));
  }

  int cols() const { return cols_; }
  int rows() const { return rows_; }

  const std::vector<std::vector<CellState>>& get() const { return grid_; }

  int place_piece(int column, CellState piece)
  {
    if (!is_column_valid_(column)) {
      throw std::logic_error("Invalid column!");
    }
    if (is_piece_empty_(piece)) {
      throw std::logic_error("Piece not be empty!");
    }
    // piece will be placed into column and lowest empty row
    for (int row = rows_ - 1; row >= 0; --row) {
      if (is_piece_empty_(grid_[row][column])) {
        grid_[row][column] = piece;
        return row;
      }
    }
    return -1;
  }

private:
  int cols_;
  int rows_;
  std::vector<std::vector<CellState>> grid_;

private:
  bool is_column_valid_(int column)
  {
    if (column >= 0 && column <= cols_) {
      return true;
    }
    return false;
  }

  bool is_piece_empty_(CellState piece) { return piece == CellState::EMPTY; }
};

/**
 * @brief This class solves the problem of checking the win condition
 *
 */
class Judge {
public:
  Judge(std::shared_ptr<Grid> grid) : grid_(grid) {}

  bool check_win(int connectN, int row, int col, CellState piece)
  {
    // Check horizontal
    int count = 0;
    for (int c = 0; c < grid_->cols(); c++) {
      if (grid_->get()[row][c] == piece) {
        count++;
      } else {
        count = 0;
      }
      if (count == connectN) {
        return true;
      }
    }

    // Check vertical
    count = 0;
    for (int r = 0; r < grid_->rows(); r++) {
      if (grid_->get()[r][col] == piece) {
        count++;
      } else {
        count = 0;
      }
      if (count == connectN) {
        return true;
      }
    }

    // Check diagonal
    count = 0;
    for (int r = 0; r < grid_->rows(); r++) {
      int c = row + col - r;  // row + col = r + c, for a diagonal
      if (c >= 0 && c < grid_->cols() && grid_->get()[r][c] == piece) {
        count++;
      } else {
        count = 0;
      }
      if (count == connectN) {
        return true;
      }
    }

    // Check anti-diagonal
    count = 0;
    for (int r = 0; r < grid_->rows(); r++) {
      int c = col - row + r;  // row - col = r - c, for an anti-diagonal
      if (c >= 0 && c < grid_->cols() && grid_->get()[r][c] == piece) {
        count++;
      } else {
        count = 0;
      }
      if (count == connectN) {
        return true;
      }
    }
    return false;
  }

private:
  std::shared_ptr<Grid> grid_;
};

/**
 * @brief We can have a Player class to represent the player's piece color and name.
 *
 */
class Player {
public:
  Player(const std::string& name, CellState piece) : name_(name), piece_(piece) {}

  const std::string& name() { return name_; }

  CellState piece_color() { return piece_; }

private:
  std::string name_;
  CellState piece_;
};

namespace utils {
void print_board(const std::shared_ptr<Grid>& grid)
{
  std::cout << "Board:" << std::endl;
  const auto& cells = grid->get();
  for (int i = 0; i < cells.size(); i++) {
    std::string row = "";
    for (const CellState& piece : cells[i]) {
      if (piece == CellState::EMPTY) {
        row += "0 ";
      } else if (piece == CellState::YELLOW) {
        row += "Y ";
      } else if (piece == CellState::RED) {
        row += "R ";
      }
    }
    std::cout << row << std::endl;
  }
  std::cout << std::endl;
}
}  // namespace utils

/**
 * @brief
 *
 */
class ConnectFourGame {
public:
  ConnectFourGame(std::shared_ptr<Grid> grid, std::unique_ptr<Judge> judge,
                  std::vector<std::unique_ptr<Player>> players, int connectN, int target_score)
    : grid_(std::move(grid))
    , players_(std::move(players))
    , judge_(std::move(judge))
    , connectN_(connectN)
    , target_score_(target_score)
  {
    for (const auto& player : players_) {
      score_[player->name()] = 0;
    }
  }

  void play()
  {
    int maxScore = 0;
    std::string winner;
    while (maxScore < target_score_) {
      winner = play_round_();
      std::cout << winner << " won the round" << std::endl;
      maxScore = std::max(score_[winner], maxScore);

      grid_->reset();  // reset grid
    }
    std::cout << winner << " won the ConnectFourGame" << std::endl;
  }

private:
  std::shared_ptr<Grid> grid_;
  std::unique_ptr<Judge> judge_;
  int connectN_;
  std::vector<std::unique_ptr<Player>> players_;
  std::unordered_map<std::string, int> score_;
  int target_score_;

private:
  std::pair<int, int> play_move_(const std::unique_ptr<Player>& player)
  {
    utils::print_board(grid_);
    std::cout << player->name() << "'s turn" << std::endl;
    int colCnt = grid_->cols();

    std::cout << "Enter column between 0 and " << (colCnt - 1) << " to add piece: ";
    int moveColumn = 0;
    std::cin >> moveColumn;

    int moveRow = grid_->place_piece(moveColumn, player->piece_color());
    return {moveRow, moveColumn};
  }

  std::string play_round_()
  {
    while (true) {
      for (const auto& player : players_) {
        const auto& [row, col] = play_move_(player);
        CellState pieceColor = player->piece_color();
        if (judge_->check_win(connectN_, row, col, pieceColor)) {
          score_[player->name()] = score_[player->name()] + 1;
          return player->name();
        }
      }
    }
  }
};

int main()
{
  auto grid = std::make_shared<Grid>(6, 6);
  std::vector<std::unique_ptr<Player>> players;
  players.push_back(std::make_unique<Player>("1", CellState::YELLOW));
  players.push_back(std::make_unique<Player>("2", CellState::RED));
  auto judge = std::make_unique<Judge>(grid);
  ConnectFourGame game(grid, std::move(judge), std::move(players), 4, 1);
  game.play();
}
