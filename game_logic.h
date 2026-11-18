#pragma once
/*
 * game_logic.h — Luật chơi Caro 9x9
 *
 * Phần 1: Logic Game
 * ------------------
 * Định nghĩa kiểu dữ liệu Board và các hàm kiểm tra luật chơi.
 * Các phần khác (MiniMax, MCTS) chỉ cần #include "game_logic.h".
 *
 * Luật:
 *   - Bàn 9x9, X đi trước
 *   - Thắng khi tạo >= 5 quân liên tiếp (ngang/dọc/chéo)
 *   - Không áp dụng luật cấm đánh hai đầu hay chặn hai đầu
 *   - Hòa khi bàn đầy mà không ai thắng
 */

#include <array>
#include <vector>
#include <utility>  // std::pair

// ============================================================
// HẰNG SỐ
// ============================================================

static const int BOARD_SIZE = 9;   // Kích thước bàn
static const int WIN_LENGTH  = 5;  // Số quân liên tiếp để thắng

// Giá trị ô
static const int EMPTY    = 0;
static const int PLAYER_X = 1;
static const int PLAYER_O = 2;

// Bốn hướng kiểm tra: (ngang, dọc, chéo chính, chéo phụ)
static const int DIR_R[4] = { 0,  1,  1,  1 };
static const int DIR_C[4] = { 1,  0,  1, -1 };

// ============================================================
// KIỂU DỮ LIỆU
// ============================================================

// Board: mảng 2 chiều 9x9 lưu giá trị EMPTY / PLAYER_X / PLAYER_O
using Board = std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE>;

// Một nước đi
struct Move {
    int row;
    int col;
};

// Kết quả trận đấu
enum class GameResult {
    ONGOING,   // Ván đang tiếp diễn
    WIN_X,     // X thắng
    WIN_O,     // O thắng
    DRAW       // Hòa (đầy bàn)
};

// ============================================================
// HÀM TIỆN ÍCH
// ============================================================

// Khởi tạo bàn trống
Board createBoard();

// Trả về đối thủ của player (PLAYER_X ↔ PLAYER_O)
int opponent(int player);

// Kiểm tra ô (row, col) có nằm trong bàn không
bool inBounds(int row, int col);

// Kiểm tra nước đi (row, col) có hợp lệ không
// (trong bàn và ô đó đang trống)
bool isValidMove(const Board& board, int row, int col);

// ============================================================
// KIỂM TRA TRẠNG THÁI
// ============================================================

// Đếm số quân liên tiếp của player tính từ (row, col) theo hướng (dr, dc)
int countInDirection(const Board& board, int row, int col,
                     int dr, int dc, int player);

// Kiểm tra xem nước đi vừa đặt tại (row, col) có tạo ra chuỗi thắng không.
// Giả sử board[row][col] đã được đặt quân.
bool checkWin(const Board& board, int row, int col);

// Kiểm tra hòa: bàn đầy và không ai thắng
bool isDraw(const Board& board);

// Trả về kết quả hiện tại của ván sau nước đi (lastRow, lastCol)
GameResult getGameResult(const Board& board, int lastRow, int lastCol);

// ============================================================
// HỖ TRỢ CHO NHÓM AI (đặt tại đây để tiện dùng chung)
// ============================================================

// Lấy danh sách các ô trống nằm trong bán kính `radius` ô
// xung quanh các quân đã đặt. Nếu bàn trống, trả về tâm (4,4).
// Các nhóm MiniMax / MCTS có thể dùng hàm này làm danh sách ứng viên.
std::vector<Move> getCandidateMoves(const Board& board, int radius = 2);

// Lấy toàn bộ ô trống trên bàn (dùng khi cần duyệt hết)
std::vector<Move> getAllEmptyMoves(const Board& board);
