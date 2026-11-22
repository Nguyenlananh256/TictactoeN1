#pragma once

#include "ai.h"
#include "game.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <vector>

namespace mcts {
inline int randomIndex(int n, std::mt19937 &rng) {
    std::uniform_int_distribution<int> distribution(0, n - 1);
    return distribution(rng);
}

inline char checkWinner(const caro::Board &board) {
    for (int row = 0; row < caro::Size; ++row) {
        for (int col = 0; col < caro::Size; ++col) {
            if (board[row][col] != '\0' &&
                !caro::Game::winningLine(board, {row, col}).empty()) {
                return board[row][col];
            }
        }
    }
    return '\0';
}

inline std::vector<caro::Cell> getLegalMoves(const caro::Board &board) {
    std::vector<caro::Cell> moves;
    for (int row = 0; row < caro::Size; ++row) {
        for (int col = 0; col < caro::Size; ++col) {
            if (board[row][col] == '\0') moves.push_back({row, col});
        }
    }
    return moves;
}

struct Node {
    caro::Board board;
    char player;
    bool isTerm = false;
    Node *parent;
    std::vector<std::unique_ptr<Node>> children;
    std::vector<caro::Cell> untriedMoves;
    int visits = 0;
    double wins = 0;

    Node(caro::Board state, char nextPlayer, bool isTerminalState = false, Node *parentNode = nullptr)
        : board(state),
          player(nextPlayer),
          isTerm(isTerminalState),
          parent(parentNode) {
        if (!isTerm) {
            untriedMoves = getLegalMoves(board);
        }
    }

    bool isFullyExpanded() const { return untriedMoves.empty(); }
};

inline double calculateUCT(const Node *child, const Node *parent, double c) {
    if (child->visits == 0) {
        return std::numeric_limits<double>::infinity();
    }

    const double exploitation = child->wins / child->visits;

    const double exploration =
        c * std::sqrt(std::log(static_cast<double>(parent->visits)) /
                      child->visits);
    return exploitation + exploration;
}

inline Node *selection(Node *node, double c) {
    while (!node->isTerm && node->isFullyExpanded()) {
        Node *bestChild = nullptr;
        double bestUct = -std::numeric_limits<double>::infinity();

        for (auto &child : node->children) {
            const double uct = calculateUCT(child.get(), node, c);
            if (uct > bestUct) {
                bestUct = uct;
                bestChild = child.get();
            }
        }

        if (!bestChild) break;
        node = bestChild;
    }
    return node;
}

inline Node *expansion(Node *node, std::mt19937 &rng) {
    if (node->isTerm || node->untriedMoves.empty()) return node;

    const int index = randomIndex(static_cast<int>(node->untriedMoves.size()), rng);
    const caro::Cell move = node->untriedMoves[index];
    node->untriedMoves.erase(node->untriedMoves.begin() + index);

    caro::Board newBoard = node->board;
    newBoard[move.row][move.col] = node->player;
    const char nextPlayer = caro::other(node->player);
    std::vector<caro::Cell> winningLine = caro::Game::winningLine(newBoard, move);
    bool isTerm = (!winningLine.empty() || getLegalMoves(newBoard).empty());

    auto newNode = std::make_unique<Node>(newBoard, nextPlayer, isTerm, node);
    Node *result = newNode.get();
    node->children.push_back(std::move(newNode));
    return result;
}

inline std::optional<char> simulation(
    caro::Board board,
    char player,
    std::mt19937 &rng,
    const std::shared_ptr<std::atomic_bool> &cancelled) {

    std::vector<caro::Cell> winningLine;
    const auto winner = checkWinner(board);
    if (winner) return winner;

    std::vector<caro::Cell> moves = getLegalMoves(board);
    if (moves.empty()) return '\0';

    while (!moves.empty()) {

        if (cancelled && cancelled->load()) return std::nullopt;

        const int index = randomIndex(static_cast<int>(moves.size()), rng);
        const caro::Cell move = moves[index];
        board[move.row][move.col] = player;
        winningLine = caro::Game::winningLine(board, move);
        if (!winningLine.empty()) return player;
        moves[index] = moves.back();
        moves.pop_back();
        player = caro::other(player);
    }

    return '\0';
}

inline void backpropagation(Node *node, char winner) {
    while (node) {
        ++node->visits;
        if (winner == '\0') {
            node->wins += 0.5;
        } else if (winner == caro::other(node->player)) {
            node->wins += 1.0;
        }
        node = node->parent;
    }
}
} // namespace mcts

inline caro::AiResponse chooseMcts(const caro::AiRequest &request) {
    caro::AiResponse response;
    std::mt19937 rng(std::random_device{}());

    if (mcts::checkWinner(request.board)) {
        response.error = "Game is already finished.";
        return response;
    }

    mcts::Node root(request.board, request.player, false, nullptr);
    if (root.untriedMoves.empty()) {
        response.error = "No available moves found.";
        return response;
    }

    for (int i = 0; i < request.options.iterations; ++i) {
        if (request.cancelled && request.cancelled->load()) {
            response.error = "Operation cancelled.";
            return response;
        }

        mcts::Node *node = mcts::selection(&root, request.options.c);
        node = mcts::expansion(node, rng);

        const auto winner = mcts::simulation(node->board, node->player, rng, request.cancelled);
        if (!winner.has_value()) {
            response.error = "Operation cancelled.";
            return response;
        }

        mcts::backpropagation(node, *winner);
    } 

    if (request.cancelled && request.cancelled->load()) {
        response.error = "Operation cancelled.";
        return response;
    }

    mcts::Node *bestChild = nullptr;
    int maxVisits = -1;
    for (auto &child : root.children) {
        if (child->visits > maxVisits) {
            maxVisits = child->visits;
            bestChild = child.get();
        }
    }

    if (!bestChild) {
        response.error = "No best child found.";
        return response;
    }

    for (int row = 0; row < caro::Size; ++row) {
        for (int col = 0; col < caro::Size; ++col) {
            if (request.board[row][col] == '\0' &&
                bestChild->board[row][col] == request.player) {
                response.cell = {row, col};
                return response;
            }
        }
    }

    response.error = "No available move found.";
    return response;
}
