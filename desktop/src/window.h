#pragma once
#include "ai.h"
#include <QMainWindow>
#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <memory>
class BoardWidget; class QComboBox; class QSpinBox; class QLabel; class QPushButton; class QListWidget; class QWidget;

class CaroWindow : public QMainWindow {
public:
    explicit CaroWindow(QWidget *parent = nullptr);
    ~CaroWindow() override;
    void registerAI(const QString &name, caro::AiProvider provider);
    caro::Game gameSnapshot() const { return game_; }
private:
    struct Config {
        int mode = 0; // 0: human/human, 1: human/AI, 2: AI/AI
        char human = 'X'; QString xAgent = "minimax", oAgent = "mcts"; caro::Options options;
    };
    Config readConfig() const;
    bool isAI(char player, const Config &config) const;
    QString agent(char player) const;
    void updateSettings();
    void startGame(Config config);
    void requestReset(Config config);
    void render();
    void place(caro::Cell cell, std::optional<caro::Metrics> metrics = std::nullopt);
    void scheduleAI();
    void cancelAI();
    void undo();
    void saveJson();
    void updateClock();

    caro::Game game_; Config config_;
    QMap<QString, caro::AiProvider> providers_;
    std::shared_ptr<std::atomic_bool> cancellation_;
    quint64 generation_ = 0;
    bool busy_ = false, paused_ = false;
    QString error_;
    QElapsedTimer clock_;
    qint64 finalElapsed_ = -1;
    BoardWidget *board_;
    QComboBox *mode_, *human_, *xAgent_, *oAgent_, *exploration_, *iterations_;
    QSpinBox *depth_, *candidates_;
    QWidget *humanSettings_, *aiSettings_, *xSettings_, *oSettings_;
    QLabel *note_, *status_, *count_, *moves_, *elapsed_, *time_, *metricDepth_, *nodes_, *empty_, *title_;
    QLabel *xPlayer_, *oPlayer_;
    QPushButton *undo_, *pause_, *save_;
    QListWidget *history_;
};
