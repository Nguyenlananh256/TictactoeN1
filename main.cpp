/*
 * main.cpp — Chương trình thử nghiệm logic game (console)
 *
 * Compile: g++ -std=c++17 -o demo.exe main.cpp game_logic.cpp
 * Run:     demo.exe
 *
 * Mục đích: kiểm tra các hàm trong game_logic.h hoạt động đúng
 * trước khi bàn giao cho nhóm MiniMax và MCTS.
 */

#include "game_logic.h"
#include <iostream>
#include <string>

// In bàn cờ ra console
void printBoard(const Board& board) {
    std::cout << "   ";
    for (int c = 0; c < BOARD_SIZE; ++c)
        std::cout << (char)('A' + c) << " ";
    std::cout << "\n";

    for (int r = 0; r < BOARD_SIZE; ++r) {
        std::cout << (r + 1 < 10 ? " " : "") << (r + 1) << " ";
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if      (board[r][c] == PLAYER_X) std::cout << "X ";
            else if (board[r][c] == PLAYER_O) std::cout << "O ";
            else                              std::cout << ". ";
        }
        std::cout << "\n";
    }
}

// Đọc nước đi từ bàn phím (ví dụ: "E5")
bool parseInput(const std::string& input, int& row, int& col) {
    if (input.size() < 2) return false;
    col = (char)toupper(input[0]) - 'A';
    try { row = std::stoi(input.substr(1)) - 1; }
    catch (...) { return false; }
    return inBounds(row, col);
}

int main() {
    std::cout << "=== Caro 9x9 — Logic Demo ===\n";
    std::cout << "Nhap nuoc di theo kieu 'E5'. Nhap 'q' de thoat.\n\n";

    Board board = createBoard();
    int currentPlayer = PLAYER_X;
    int lastRow = -1, lastCol = -1;

    while (true) {
        printBoard(board);

        // Hiện kết quả nếu có
        if (lastRow >= 0) {
            GameResult result = getGameResult(board, lastRow, lastCol);
            if (result == GameResult::WIN_X) { std::cout << "\n*** X THANG! ***\n"; break; }
            if (result == GameResult::WIN_O) { std::cout << "\n*** O THANG! ***\n"; break; }
            if (result == GameResult::DRAW)  { std::cout << "\n*** HOA! ***\n";      break; }
        }

        std::string symbol = (currentPlayer == PLAYER_X) ? "X" : "O";
        std::cout << "\nLuot cua " << symbol << ": ";

        std::string input;
        std::cin >> input;
        if (input == "q") break;

        int row, col;
        if (!parseInput(input, row, col)) {
            std::cout << "Nhap sai dinh dang! Vi du: E5\n";
            continue;
        }
        if (!isValidMove(board, row, col)) {
            std::cout << "O da co quan hoac ngoai ban! Chon o khac.\n";
            continue;
        }

        board[row][col] = currentPlayer;
        lastRow = row;
        lastCol = col;
        currentPlayer = opponent(currentPlayer);
    }

    return 0;
}
