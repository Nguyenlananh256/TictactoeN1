#include "window.h"
#include "board_widget.h"
#include <QApplication>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QElapsedTimer>
#include <QThread>
#include <cstdlib>
#include <iostream>

void require(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void pump(int milliseconds) {
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < milliseconds) { QApplication::processEvents(); QThread::msleep(1); }
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    CaroWindow window;
    BoardWidget *board = nullptr;
    for (auto *child : window.findChildren<QWidget *>()) if (auto *candidate = dynamic_cast<BoardWidget *>(child)) board = candidate;
    auto *undo = window.findChild<QPushButton *>("undo");
    auto *mode = window.findChild<QComboBox *>("gameMode");
    auto *newGame = window.findChild<QPushButton *>("primary");
    auto *status = window.findChild<QLabel *>("gameStatus");
    require(board && undo && mode && newGame && status, "UI controls constructed");
    board->onMove({4, 4}); board->onMove({4, 5});
    require(window.gameSnapshot().moves.size() == 2, "human moves reach model");
    undo->click(); undo->click();
    require(window.gameSnapshot().moves.empty(), "undo button restores empty board");
    mode->setCurrentIndex(1); newGame->click();
    board->onMove({4, 4});
    require(status->text().contains("not connected"), "missing AI is explicit");
    undo->click();
    window.registerAI("mcts", [](const caro::AiRequest &request) {
        for (int i = 0; i < 30 && !request.cancelled->load(); ++i) QThread::msleep(5);
        return caro::AiResponse{{0, 0}, std::nullopt, 42, {}};
    });
    board->onMove({4, 4}); undo->click(); pump(250);
    require(window.gameSnapshot().moves.empty(), "canceled AI cannot write a stale move");
    board->onMove({4, 4}); pump(500);
    require(window.gameSnapshot().moves.size() == 2 && window.gameSnapshot().board[0][0] == 'O', "worker result reaches board");
    require(window.gameSnapshot().moves.back().metrics->nodes == 42, "provider metrics preserved");
    undo->click();
    require(window.gameSnapshot().moves.empty(), "human vs AI undo removes both moves");
    window.registerAI("mcts", [](const caro::AiRequest &) { return caro::AiResponse{{99, 99}, {}, {}, {}}; });
    board->onMove({4, 4}); pump(250);
    require(window.gameSnapshot().moves.size() == 1 && status->text().contains("invalid move"), "invalid AI result rejected");
    std::cout << "All UI integration checks passed.\n";
}
