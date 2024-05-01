# Requirements

Task: Need to develop the game of Snake and Ladders

## Functional

Game rules:

1. N players are playing in the game.
2. Each player can dice a roll and move сhip on the board.
3. The dice has values ranging from 1 to 6. The value of the dice means of times
   the chip is moved on the board.
4. If a player moves to a square with a snake head, then the player moves to a
   square with a snake tail.
5. If a player moves to a square with a ladder tail, then the player moves to a
   square with a ladder head.
6. Players are moved by queue.
7. Player win, if first moved into the end of the board.

The board: Size is 10 x 10. Contains 10 snakes and ladders. The head of the
snake lies above the tail. The head of the ladder lies below the tail.

# Entities & Relationships

Entities: Player, Board, Game.

## Board:

Responsibilities:
1. The board solves the task of storing the state of the figures.
2. The board can report which position the player should move to if he got into a position with snake, ladder or out of the board.

State: size, snakes positions, ladders positions, chips positions

Behavior:
1. Return position of snake tail, if player moved to position with snake head.
2. Return position of ladder head, if player moved to position with ladder tail.
3. Return position by formula: size - (pos - size), if player moved to out the board.

## Player:

Responsibilities:
1. Player solves the task of storing information and moves the ship on the board.
2. The player can ask the board what position to move to if the player hits a snake head, the start of a ladder or goes out of bounds.

State: id, position on the board

Behavior: move chip by game rules, say position on the board

## Game:

Responsibilities: The controller of the game -- managing the interaction of the players and the board, determine the winner.

State: Players, Board

Behavior: Players addition, run the game,
determine the winner
