#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "tictactoe/player.h"
#include "tictactoe/bot.h"
#include "tictactoe/game.h"

struct MockUser : public User {
  MockUser(const std::string& name) : User(name) {}
  MOCK_METHOD(int, get_cell_num, (), (override));
};

struct MockBot : public Bot {
  MOCK_METHOD(int, get_cell_num, (), (override));
};

struct SilentTicTacToe : TicTacToe {
  SilentTicTacToe(std::vector<std::unique_ptr<IPlayer>> players) : TicTacToe(std::move(players)) {}
  void print_board_() {}
};

struct TicTacToeFixture : public ::testing::Test {
  void SetUp()
  {
    mock_user = std::make_unique<MockUser>("mockuser");
    mock_bot = std::make_unique<MockBot>();
  }

  void TearDown() {}

  std::unique_ptr<MockUser> mock_user;
  std::unique_ptr<MockBot> mock_bot;
};

TEST_F(TicTacToeFixture, UserWinFirstRowTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(1))
      .WillOnce(::testing::Return(2));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(3)).WillOnce(::testing::Return(4));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinSecondRowTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(3))
      .WillOnce(::testing::Return(4))
      .WillOnce(::testing::Return(5));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(6)).WillOnce(::testing::Return(7));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinThirdRowTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(6))
      .WillOnce(::testing::Return(7))
      .WillOnce(::testing::Return(8));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(3)).WillOnce(::testing::Return(4));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinFirstColTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(3))
      .WillOnce(::testing::Return(6));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(1)).WillOnce(::testing::Return(4));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinSecondColTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(1))
      .WillOnce(::testing::Return(4))
      .WillOnce(::testing::Return(7));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(2)).WillOnce(::testing::Return(5));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinThirdColTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(2))
      .WillOnce(::testing::Return(5))
      .WillOnce(::testing::Return(8));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(0)).WillOnce(::testing::Return(3));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinFirstDiagonalTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(4))
      .WillOnce(::testing::Return(8));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(3)).WillOnce(::testing::Return(6));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}

TEST_F(TicTacToeFixture, UserWinSecondDiagonalTest)
{
  EXPECT_CALL(*mock_user, get_cell_num)
      .WillOnce(::testing::Return(2))
      .WillOnce(::testing::Return(4))
      .WillOnce(::testing::Return(6));
  EXPECT_CALL(*mock_bot, get_cell_num).WillOnce(::testing::Return(7)).WillOnce(::testing::Return(8));
  std::vector<std::unique_ptr<IPlayer>> players;
  players.push_back(std::move(mock_user));
  players.push_back(std::move(mock_bot));
  SilentTicTacToe tictactoe(std::move(players));
  auto winner = tictactoe.play();
  EXPECT_EQ("mockuser", winner->name());
}
