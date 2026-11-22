#pragma once
#include "ai.h"
#include <random>
#include <vector>


caro::AiResponse chooseRandom(const caro::AiRequest &request) {
    std::vector<caro::Cell> availableMoves;

    for (int row = 0; row < caro::Size; ++row) {
        for (int col = 0; col < caro::Size; ++col) {
            if (request.board[row][col] == '\0') {
                availableMoves.push_back({row, col});
            }
        }
    }

    caro::AiResponse response;

    if (availableMoves.empty()) {
        response.error = "No available moves.";
        return response;
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(
        0,
        availableMoves.size() - 1
    );

    response.cell = availableMoves[distribution(rng)];
    return response;
}