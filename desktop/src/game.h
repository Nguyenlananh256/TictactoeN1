#pragma once
#include <array>
#include <optional>
#include <vector>

namespace caro {
constexpr int Size = 9;
using Board = std::array<std::array<char, Size>, Size>;
struct Cell { int row = -1; int col = -1; };
inline bool operator==(Cell a, Cell b) { return a.row == b.row && a.col == b.col; }
inline char other(char player) { return player == 'X' ? 'O' : 'X'; }
inline bool valid(Cell cell) { return cell.row >= 0 && cell.row < Size && cell.col >= 0 && cell.col < Size; }
struct Metrics {
    double timeMs = 0;
    std::optional<int> depth;
    std::optional<long long> nodes;
};
struct Move { Cell cell; char player; std::optional<Metrics> metrics; };

class Game {
public:
    Board board{}; // '\0' is an empty cell.
    std::vector<Move> moves;
    std::vector<Cell> winning;
    char turn = 'X';
    char winner = '\0';
    bool finished = false;

    static std::vector<Cell> winningLine(const Board &board, Cell origin) {
        if (!valid(origin) || board[origin.row][origin.col] == '\0') return {};
        const char player = board[origin.row][origin.col];
        for (const Cell direction : {Cell{0, 1}, Cell{1, 0}, Cell{1, 1}, Cell{1, -1}}) {
            std::vector<Cell> line{origin};
            for (int sign : {-1, 1}) {
                Cell cell{origin.row + direction.row * sign, origin.col + direction.col * sign};
                while (valid(cell) && board[cell.row][cell.col] == player) {
                    line.push_back(cell);
                    cell.row += direction.row * sign;
                    cell.col += direction.col * sign;
                }
            }
            if (line.size() >= 5) return line;
        }
        return {};
    }
    bool play(Cell cell, std::optional<Metrics> metrics = std::nullopt) {
        if (finished || !valid(cell) || board[cell.row][cell.col] != '\0') return false;
        board[cell.row][cell.col] = turn;
        moves.push_back({cell, turn, metrics});
        winning = winningLine(board, cell);
        if (!winning.empty()) { winner = turn; finished = true; }
        else if (moves.size() == Size * Size) finished = true;
        else turn = other(turn);
        return true;
    }
    bool undo() {
        if (moves.empty()) return false;
        const auto move = moves.back();
        moves.pop_back(); board[move.cell.row][move.cell.col] = '\0'; turn = move.player;
        winning.clear(); winner = '\0'; finished = false;
        return true;
    }
};
}
