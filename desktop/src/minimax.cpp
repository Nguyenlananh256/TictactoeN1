#include "minimax.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace caro {
namespace {

constexpr int BoardSize = Size;
constexpr long long MateScore = 1'000'000'000LL;
constexpr long long InfinityScore = std::numeric_limits<long long>::max() / 4;
constexpr long long NodeBudget = 300'000;
constexpr int HardCandidateLimit = 16;
constexpr int MaxSearchDepth = 10;

const std::array<Cell, 4> Directions{{
    Cell{0, 1}, Cell{1, 0}, Cell{1, 1}, Cell{1, -1}
}};

bool isCancelled(const AiRequest &request) {
    return request.cancelled && request.cancelled->load();
}

bool hasFive(const Board &board, char player) {
    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            if (board[row][col] != player) continue;

            for (const Cell direction : Directions) {
                int count = 1;
                int r = row + direction.row;
                int c = col + direction.col;

                while (r >= 0 && r < BoardSize && c >= 0 && c < BoardSize
                       && board[r][c] == player) {
                    ++count;
                    r += direction.row;
                    c += direction.col;
                }

                if (count >= 5) return true;
            }
        }
    }
    return false;
}

long long patternValue(int count, int emptyCount) {
    if (count <= 0 || emptyCount <= 0) return 0;

    static constexpr long long values[] = {
        0, 2, 18, 180, 18'000, 0
    };

    long long value = values[count];
    if (count == 4) value = 18'000;
    if (count == 3 && emptyCount == 2) value = 260;
    if (count == 2 && emptyCount == 2) value = 24;
    return value;
}

long long evaluateFor(const Board &board, char player) {
    const char opponent = other(player);
    long long score = 0;

    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            for (const Cell direction : Directions) {
                const int endRow = row + 4 * direction.row;
                const int endCol = col + 4 * direction.col;
                if (endRow < 0 || endRow >= BoardSize ||
                    endCol < 0 || endCol >= BoardSize) {
                    continue;
                }

                int ownCount = 0;
                int opponentCount = 0;
                int emptyCount = 0;

                for (int offset = 0; offset < 5; ++offset) {
                    const char value = board[row + offset * direction.row]
                                            [col + offset * direction.col];
                    if (value == player) ++ownCount;
                    else if (value == opponent) ++opponentCount;
                    else ++emptyCount;
                }

                if (ownCount > 0 && opponentCount == 0)
                    score += patternValue(ownCount, emptyCount);
                else if (opponentCount > 0 && ownCount == 0)
                    score -= patternValue(opponentCount, emptyCount);
            }
        }
    }

    return score;
}

long long evaluate(const Board &board, char rootPlayer) {
    if (hasFive(board, rootPlayer)) return MateScore;
    if (hasFive(board, other(rootPlayer))) return -MateScore;
    return evaluateFor(board, rootPlayer);
}

struct Candidate {
    Cell cell;
    long long orderingScore = 0;
};

bool wouldWinAt(const Board &board, Cell cell, char player) {
    if (!valid(cell) || board[cell.row][cell.col] != '\0') return false;

    Board next = board;
    next[cell.row][cell.col] = player;
    return hasFive(next, player);
}

std::vector<Candidate> generateCandidates(
    const Board &board, char player, int candidateLimit) {

    std::array<std::array<bool, BoardSize>, BoardSize> marked{};
    std::vector<Candidate> candidates;
    bool hasAnyMark = false;

    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            if (board[row][col] == '\0') continue;
            hasAnyMark = true;

            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    const int r = row + dr;
                    const int c = col + dc;
                    if (r < 0 || r >= BoardSize || c < 0 || c >= BoardSize)
                        continue;
                    if (board[r][c] == '\0') marked[r][c] = true;
                }
            }
        }
    }

    if (!hasAnyMark) {
        const Cell center{BoardSize / 2, BoardSize / 2};
        return {{center, 0}};
    }

    const char opponent = other(player);
    for (int row = 0; row < BoardSize; ++row) {
        for (int col = 0; col < BoardSize; ++col) {
            if (!marked[row][col]) continue;

            const Cell cell{row, col};
            const bool blocksImmediateWin = wouldWinAt(board, cell, opponent);

            Board next = board;
            next[row][col] = player;

            long long score = evaluateFor(next, player) - evaluateFor(next, opponent);

            if (hasFive(next, player)) score += MateScore;
            else if (blocksImmediateWin) score += MateScore / 2;

            candidates.push_back({cell, score});
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(),
        [](const Candidate &left, const Candidate &right) {
            return left.orderingScore > right.orderingScore;
        });

    if (static_cast<int>(candidates.size()) > candidateLimit)
        candidates.resize(static_cast<std::size_t>(candidateLimit));

    return candidates;
}

struct SearchContext {
    const AiRequest &request;
    char rootPlayer;
    int candidateLimit;
    long long nodes = 0;
    bool stopped = false;
};

long long minimax(
    Board &board,
    char currentPlayer,
    int depth,
    int ply,
    long long alpha,
    long long beta,
    SearchContext &context) {

    if (isCancelled(context.request)) {
        context.stopped = true;
        return 0;
    }

    ++context.nodes;
    if (context.nodes > NodeBudget) {
        context.stopped = true;
        return evaluate(board, context.rootPlayer);
    }

    if (hasFive(board, context.rootPlayer))
        return MateScore - ply;
    if (hasFive(board, other(context.rootPlayer)))
        return -MateScore + ply;

    if (depth <= 0) return evaluateFor(board, context.rootPlayer);

    auto candidates = generateCandidates(board, currentPlayer,
                                         context.candidateLimit);
    if (candidates.empty()) return 0;

    const bool maximizing = currentPlayer == context.rootPlayer;
    long long best = maximizing ? -InfinityScore : InfinityScore;

    for (const Candidate &candidate : candidates) {
        if (isCancelled(context.request) || context.stopped) {
            context.stopped = true;
            break;
        }

        const Cell cell = candidate.cell;
        board[cell.row][cell.col] = currentPlayer;

        const long long value = minimax(
            board, other(currentPlayer), depth - 1, ply + 1,
            alpha, beta, context);

        board[cell.row][cell.col] = '\0';

        if (maximizing) {
            best = std::max(best, value);
            alpha = std::max(alpha, best);
        } else {
            best = std::min(best, value);
            beta = std::min(beta, best);
        }

        if (beta <= alpha) break;
    }

    return best;
}

} // namespace

AiResponse chooseMiniMaxMove(const AiRequest &request) {
    AiResponse response;

    if (request.player != 'X' && request.player != 'O') {
        response.error = "MiniMax received an invalid player mark.";
        return response;
    }

    if (isCancelled(request)) return response;

    int requestedDepth = std::max(2, request.options.depth);
    requestedDepth = std::min(requestedDepth, MaxSearchDepth);

    int candidateLimit = std::max(1, request.options.candidateLimit);
    candidateLimit = std::min(candidateLimit, HardCandidateLimit);

    Board board = request.board;
    bool hasEmptyCell = false;
    for (const auto &row : board)
        for (char cell : row)
            if (cell == '\0') hasEmptyCell = true;

    if (!hasEmptyCell) {
        response.error = "No legal move is available.";
        return response;
    }

    auto rootCandidates = generateCandidates(board, request.player, candidateLimit);
    if (rootCandidates.empty()) {
        response.error = "No legal candidate move was found.";
        return response;
    }

    Cell bestMove = rootCandidates.front().cell;
    int completedDepth = 0;
    SearchContext context{request, request.player, candidateLimit};

    for (int depth = 2; depth <= requestedDepth; ++depth) {
        if (isCancelled(request) || context.stopped) break;

        long long bestScore = -InfinityScore;
        Cell depthBestMove = bestMove;
        long long alpha = -InfinityScore;
        const long long beta = InfinityScore;
        bool completedIteration = true;

        for (const Candidate &candidate : rootCandidates) {
            if (isCancelled(request) || context.stopped) {
                completedIteration = false;
                break;
            }

            const Cell cell = candidate.cell;
            board[cell.row][cell.col] = request.player;

            const long long score = minimax(
                board, other(request.player), depth - 1, 1,
                alpha, beta, context);

            board[cell.row][cell.col] = '\0';

            if (context.stopped) {
                completedIteration = false;
                break;
            }

            if (score > bestScore) {
                bestScore = score;
                depthBestMove = cell;
            }
            alpha = std::max(alpha, bestScore);
        }

        if (!completedIteration) break;

        bestMove = depthBestMove;
        completedDepth = depth;
        if (bestScore >= MateScore - 100) break;
    }

    if (isCancelled(request)) return AiResponse{};

    response.cell = bestMove;
    response.depth = completedDepth > 0 ? completedDepth : 2;
    response.nodes = context.nodes;
    return response;
}

} // namespace caro
