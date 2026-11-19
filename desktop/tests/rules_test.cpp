#include "game.h"
#include <cstdlib>
#include <iostream>

void require(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
int main() {
    for (const auto direction : {caro::Cell{0, 1}, caro::Cell{1, 0}, caro::Cell{1, 1}, caro::Cell{1, -1}}) {
        caro::Board board{}; const int col = direction.col < 0 ? 8 : 0;
        for (int i = 0; i < 5; ++i) board[i * direction.row][col + i * direction.col] = 'X';
        require(caro::Game::winningLine(board, {0, col}).size() == 5, "five in every direction");
    }
    caro::Board board{};
    for (int col = 1; col <= 6; ++col) board[4][col] = 'O';
    board[4][0] = board[4][7] = 'X';
    require(caro::Game::winningLine(board, {4, 4}).size() == 6, "overline wins even with blocked ends");
    require(caro::Game::winningLine(board, {-1, 0}).empty(), "invalid cell is safe");
    caro::Game game;
    require(!game.play({-1, 0}), "reject outside board");
    require(game.play({0, 0}), "first move");
    require(!game.play({0, 0}) && game.turn == 'O', "occupied cell does not advance turn");
    require(game.undo() && game.turn == 'X' && !game.board[0][0], "undo restores state");
    for (int col = 0; col < 4; ++col) { require(game.play({0, col}), "X move"); require(game.play({8, col}), "O move"); }
    require(!game.finished, "four is not a win");
    require(game.play({0, 4}) && game.finished && game.winner == 'X', "game detects win");
    require(!game.play({2, 2}), "no moves after finish");
    require(game.undo() && !game.finished && !game.winner && game.winning.empty() && game.turn == 'X', "undo reopens finished game");
    // This full pattern has no five in any of the four directions.
    caro::Game draw;
    for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) draw.board[r][c] = ((r + 2 * c) % 4 < 2) ? 'X' : 'O';
    for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) require(caro::Game::winningLine(draw.board, {r, c}).empty(), "draw fixture has no winner");
    const char last = draw.board[8][8]; draw.board[8][8] = '\0'; draw.turn = last;
    draw.moves.resize(80, {{0, 0}, 'X', std::nullopt});
    require(draw.play({8, 8}) && draw.finished && !draw.winner, "full board draw");
    std::cout << "All board rule checks passed.\n";
}
