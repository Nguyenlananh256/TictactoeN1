#pragma once
#include "game.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace caro {
struct Options { int depth = 2; int candidateLimit = 12; double c = 1.4; int iterations = 300; };
struct AiRequest {
    Board board;
    char player;
    Options options;
    std::shared_ptr<std::atomic_bool> cancelled;
};
struct AiResponse {
    Cell cell;
    std::optional<int> depth;
    std::optional<long long> nodes;
    std::string error;
};
// Called on a worker thread. Do not access widgets from this function.
// Check request.cancelled periodically and return promptly when true.
using AiProvider = std::function<AiResponse(const AiRequest &)>;
}
