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
#include <unordered_map>
#include <vector>

enum class CellState { EMPTY, YELLOW, RED };

/**
 * @brief We will need a Grid class to maintain the state of the 2-D board
 * The board cell can be empty, yellow (occupied by Player 1) or red (occupied by Player
 * 2) The grid will also be responsible for checking for a win condition
 *
 */
class Grid {
public:
    Grid(int cols, int rows) : cols_(cols), rows_(rows) {
        grid_.resize(rows, std::vector<CellState>(cols, CellState::EMPTY));
    }

    void reset() {
        grid_.clear();
        grid_.resize(rows_, std::vector<CellState>(cols_, CellState::EMPTY));
    }

    int cols() const { return cols_; }

    const std::vector<std::vector<CellState>> &get() const { return grid_; }

    int place_piece(int column, CellState piece) {
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

    bool check_win(int connectN, int row, int col, CellState piece) {
        // Check horizontal
        int count = 0;
        for (int c = 0; c < cols_; c++) {
            if (grid_[row][c] == piece) {
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
        for (int r = 0; r < this->rows_; r++) {
            if (grid_[r][col] == piece) {
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
        for (int r = 0; r < rows_; r++) {
            int c = row + col - r;  // row + col = r + c, for a diagonal
            if (c >= 0 && c < cols_ && grid_[r][c] == piece) {
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
        for (int r = 0; r < rows_; r++) {
            int c = col - row + r;  // row - col = r - c, for an anti-diagonal
            if (c >= 0 && c < cols_ && grid_[r][c] == piece) {
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
    int cols_;
    int rows_;
    std::vector<std::vector<CellState>> grid_;

private:
    bool is_column_valid_(int column) {
        if (column >= 0 && column <= cols_) {
            return true;
        }
        return false;
    }

    bool is_piece_empty_(CellState piece) { return piece == CellState::EMPTY; }
};

/**
 * @brief We can have a Player class to represent the player's piece color and name.
 *
 */
class Player {
public:
    Player(const std::string &name, CellState piece) : name_(name), piece_(piece) {}

    const std::string &name() { return name_; }

    CellState piece_color() { return piece_; }

private:
    std::string name_;
    CellState piece_;
};

class ConnectFourGame {
public:
    ConnectFourGame(Grid *grid, int connectN, int targetScore) {
        this->grid = grid;
        this->connectN = connectN;
        this->targetScore = targetScore;

        this->players = std::vector<Player *>{new Player("Player 1", CellState::YELLOW),
                                              new Player("Player 2", CellState::RED)};

        this->score = std::unordered_map<std::string, int>();
        for (Player *player : this->players) {
            this->score[player->name()] = 0;
        }
    }

    void printBoard() {
        std::cout << "Board:" << std::endl;
        std::vector<std::vector<CellState>> grid = this->grid->get();
        for (int i = 0; i < grid.size(); i++) {
            std::string row = "";
            for (const CellState &piece : grid[i]) {
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

    std::vector<int> playMove(Player *player) {
        printBoard();
        std::cout << player->name() << "'s turn" << std::endl;
        int colCnt = this->grid->cols();

        std::cout << "Enter column between 0 and " << (colCnt - 1) << " to add piece: ";
        int moveColumn = 0;
        std::cin >> moveColumn;

        int moveRow = this->grid->place_piece(moveColumn, player->piece_color());
        return std::vector<int>{moveRow, moveColumn};
    }

    Player *playRound() {
        while (true) {
            for (Player *player : this->players) {
                std::vector<int> pos = playMove(player);
                int row = pos[0];
                int col = pos[1];
                CellState pieceColor = player->piece_color();
                if (this->grid->check_win(this->connectN, row, col, pieceColor)) {
                    this->score[player->name()] = this->score[player->name()] + 1;
                    return player;
                }
            }
        }
    }

    void play() {
        int maxScore = 0;
        Player *winner = nullptr;
        while (maxScore < this->targetScore) {
            winner = playRound();
            std::cout << winner->name() << " won the round" << std::endl;
            maxScore = std::max(this->score[winner->name()], maxScore);

            this->grid->reset();  // reset grid
        }
        std::cout << winner->name() << " won the ConnectFourGame" << std::endl;
    }

private:
    Grid *grid;
    int connectN;
    std::vector<Player *> players;
    std::unordered_map<std::string, int> score;
    int targetScore;
};

int main() {
    Grid *grid = new Grid(6, 6);
    ConnectFourGame gane(grid, 4, 10);
    gane.play();
}
