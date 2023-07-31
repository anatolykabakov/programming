#include "gtest/gtest.h"  // we will add the path to C preprocessor later
#include "snake_and_ladder/board.h"

TEST(BoardTest, SnakeLaddersTest)
{
  Board b(10);
  EXPECT_FALSE(b.out_of_size(80));
  EXPECT_FALSE(b.out_of_size(100));
  EXPECT_TRUE(b.out_of_size(0));
  EXPECT_TRUE(b.out_of_size(-1));
}
