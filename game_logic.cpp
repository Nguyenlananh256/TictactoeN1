/*
 * game_logic.cpp — Cài đặt luật chơi Caro 9x9
 *
 * Phần 1: Logic Game
 */

#include "game_logic.h"

// ============================================================
// HÀM TIỆN ÍCH
// ============================================================

Board createBoard() {
    Board board;
    for (auto& row : board)
        row.fill(EMPTY);
    return board;
}

int opponent(int player) {
    return (player == PLAYER_X) ? PLAYER_O : PLAYER_X;
}

bool inBounds(int row, int col) {
    return row >= 0 && row < BOARD_SIZE
        && col >= 0 && col < BOARD_SIZE;
}

bool isValidMove(const Board& board, int row, int col) {
    return inBounds(row, col) && board[row][col] == EMPTY;
}

// ============================================================
// KIỂM TRA TRẠNG THÁI
// ============================================================

int countInDirection(const Board& board, int row, int col,
                     int dr, int dc, int player) {
    int count = 0;
    int r = row + dr;
    int c = col + dc;
    while (inBounds(r, c) && board[r][c] == player) {
        ++count;
        r += dr;
        c += dc;
    }
    return count;
}

bool checkWin(const Board& board, int row, int col) {
    int player = board[row][col];
    if (player == EMPTY) return false;

    // Kiểm tra 4 hướng: với mỗi hướng, đếm cả hai chiều
    for (int d = 0; d < 4; ++d) {
        int total = 1
            + countInDirection(board, row, col,  DIR_R[d],  DIR_C[d], player)
            + countInDirection(board, row, col, -DIR_R[d], -DIR_C[d], player);
        if (total >= WIN_LENGTH)
            return true;
    }
    return false;
}

bool isDraw(const Board& board) {
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (board[r][c] == EMPTY)
                return false;
    return true;  // Bàn đầy
}

GameResult getGameResult(const Board& board, int lastRow, int lastCol) {
    if (board[lastRow][lastCol] == EMPTY)
        return GameResult::ONGOING;

    if (checkWin(board, lastRow, lastCol)) {
        return (board[lastRow][lastCol] == PLAYER_X)
               ? GameResult::WIN_X
               : GameResult::WIN_O;
    }

    if (isDraw(board))
        return GameResult::DRAW;

    return GameResult::ONGOING;
}

// ============================================================
// HỖ TRỢ CHO NHÓM AI
// ============================================================

std::vector<Move> getCandidateMoves(const Board& board, int radius) {
    // Nếu bàn trống → trả về tâm bàn
    bool hasAny = false;
    for (int r = 0; r < BOARD_SIZE && !hasAny; ++r)
        for (int c = 0; c < BOARD_SIZE && !hasAny; ++c)
            if (board[r][c] != EMPTY) hasAny = true;

    if (!hasAny)
        return { {4, 4} };

    // Đánh dấu các ô trống nằm trong bán kính `radius` của quân đã đặt
    std::array<std::array<bool, BOARD_SIZE>, BOARD_SIZE> marked{};
    for (auto& row : marked) row.fill(false);

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] == EMPTY) continue;
            // Quét vùng lân cận
            for (int dr = -radius; dr <= radius; ++dr) {
                for (int dc = -radius; dc <= radius; ++dc) {
                    int nr = r + dr, nc = c + dc;
                    if (inBounds(nr, nc) && board[nr][nc] == EMPTY)
                        marked[nr][nc] = true;
                }
            }
        }
    }

    std::vector<Move> candidates;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (marked[r][c])
                candidates.push_back({r, c});

    return candidates.empty() ? std::vector<Move>{{4, 4}} : candidates;
}

std::vector<Move> getAllEmptyMoves(const Board& board) {
    std::vector<Move> moves;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (board[r][c] == EMPTY)
                moves.push_back({r, c});
    return moves;
}
