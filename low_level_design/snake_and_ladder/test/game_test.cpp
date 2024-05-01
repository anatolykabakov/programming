#include "gtest/gtest.h"
#include "snake_and_ladder/board.h"
#include "snake_and_ladder/player.h"
#include "snake_and_ladder/game.h"

TEST(SnakeLaddersTest, BoardTest)
{
  Board b(100);
  EXPECT_EQ(b.out_of_size(80), 80);
  EXPECT_EQ(b.out_of_size(100), 100);
  EXPECT_EQ(b.out_of_size(0), 0);
  EXPECT_EQ(b.out_of_size(101), 99);
  EXPECT_EQ(b.out_of_size(-1), -1);

  EXPECT_EQ(b.on_snake(95), 1);
  EXPECT_EQ(b.on_snake(19), 5);
  EXPECT_EQ(b.on_snake(31), 8);

  EXPECT_EQ(b.on_ladder(3), 45);
  EXPECT_EQ(b.on_ladder(7), 26);
  EXPECT_EQ(b.on_ladder(9), 28);
}

class MockPlayer : public Player {
public:
  MockPlayer(int id) : Player(id) {}

  int roll_dice() override { return 1; }
};

TEST(SnakeLaddersTest, GameTest)
{
  std::vector<Player> players = {MockPlayer{0}, MockPlayer{1}};
  Game snake_and_ladder(players);
  auto winner = snake_and_ladder.play();
  EXPECT_EQ(winner.id(), 0);
}

class MockBoard : public Board {
public:
  MockBoard(int size) : Board(size) {}

  virtual int on_snake(int pos) override { return pos + 1; }

  virtual int on_ladder(int pos) override { return pos + 1; }

  virtual int out_of_size(int pos) override { return pos + 1; }
};

TEST(SnakeLaddersTest, PlayerTest)
{
  Player p(0);
  EXPECT_EQ(p.position(), 0);
  EXPECT_EQ(p.id(), 0);

  Board b(100);
  p.move(b, 1);
  EXPECT_EQ(p.position(), 1);

  MockBoard mb(100);
  p.move(mb, 1);
  EXPECT_EQ(p.position(), 5);
}
