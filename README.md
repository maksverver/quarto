# Quarto

A command line version of the abstract board game Quarto published by Gigamic.
Includes an AI player using a Monte Carlo Tree Search / Minimax hybrid algorithm.


## Rules of the game

Quarto is played by two players using 16 pieces (shared between the players) and a board
with 16 fields, arranged in a 4x4 square.

Each piece has four attributes (color, size, shape, and filling) each of which as two values
(black and white, short and tall, round and square, flat or hollow). These values
are represented abstractly using symbols: a/b, +/-, 0/1, x/y.

The first player starts by selecting an available piece, and gives it to his opponent.
The second player then places the piece on an empty field, and selects a different piece
for player 2 to place. The players continue taking turns like this until one player wins
or they run out of pieces and the game ends.

A player wins by forming a "quarto": a row of four pieces that share at least one
common attribute (e.g. all four are black, or all four are square, etc.) When a player
completes a quarto, he may call "quarto" to win the game instead of selecting the next
piece. If the player doesn't notice he formed a quarto, his opponent may still call
"quarto" to win the game instead, but only on his next turn, before placing his piece.
If the opponent doesn't notice the quarto either, the opportunity is lost forever.

At the end of the game, when there are no more pieces to pick, player 1 can either call
quarto (if his last move formed one) or else he must pass. This gives his opponent an
opportunity to still call quarto, or else he must pass too. When both players have passed,
the game ends in a tie.


## Screenshots

        +----------------+
      4 | ..  ..  ..  .. |        a+  a+  a+  a+
        | ..  ..  ..  .. |        0x  0y  1x  1y
        |                |
      3 | ..  ..  ..  .. |        a-  a-  a-  a-
        | ..  ..  ..  .. |   ..   0x  0y  1x  1y
        |                |   ..
      2 | ..  ..  ..  .. |  --->  b+  b+  b+  b+
        | ..  ..  ..  .. |        0x  0y  1x  1y
        |                |
      1 | ..  ..  ..  .. |        b-  b-  b-  b-
        | ..  ..  ..  .. |        0x  0y  1x  1y
        +----------------+
           A   B   C   D
    Player 1 to move. (Q)uarto or select piece:


        +----------------+
      4 | ..  ..  ..  .. |        ..  a+  ..  ..
        | ..  ..  ..  .. |        ..  0y  ..  ..
        |                |
      3 | .. [a+] ..  a+ |  <---  a-  .. [..] ..
        | .. [1y] ..  1x |   a-   0x  .. [..] ..
        |                |   1x
      2 | ..  ..  ..  b+ |        ..  b+  b+  b+
        | ..  ..  ..  0x |        ..  0y  1x  1y
        |                |
      1 | a-  b-  a+  a- |        ..  b-  b-  b-
        | 1y  0x  0x  0y |        ..  0y  1x  1y
        +----------------+
       A   B   C   D
    Player 1 to move. (Q)uarto or place on field: 


## Command line interface

The game supports the following four types of moves:

  * Select a piece for the opponent to plage (e.g. `a+0x`).
  * Place the selected piece on a field (e.g. `C4`).
  * Call Quarto: `q` or `quarto`.
  * Pass with: `p` or `pass`.

The program recognizes a few more commands:

  * `a` or `ai': let the AI make the next move.
  * `h` or `history`: print a transcript of the moves played so far.
     A compact representation is also printed, which can be used to restart the game later
     by passing it as a command line argument, e.g.: `./quarto 8r7sct0u5v2n3l6`
  * `x` or `exit`: exit the game.
  
