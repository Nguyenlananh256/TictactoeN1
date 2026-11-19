#include "window.h"
#include "board_widget.h"
#include <QtWidgets>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <exception>
#include <stdexcept>
#include <utility>

namespace {
const QColor Ink("#25418f");
QString handFamily() {
    const auto families = QFontDatabase().families();
    for (const auto &name : {QStringLiteral("Patrick Hand"), QStringLiteral("Segoe Print"), QStringLiteral("Comic Sans MS"), QStringLiteral("Chalkboard")})
        if (families.contains(name)) return name;
    return QStringLiteral("Sans Serif");
}
QFont hand(int size) { return QFont(handFamily(), size); }
QString coordinate(caro::Cell cell) { return QString(QChar('A' + cell.col)) + QString::number(cell.row + 1); }
QString algorithmName(const QString &id) {
    if (id == "minimax") return "MiniMax + Alpha-Beta";
    if (id == "mcts") return "MCTS + UCT";
    return "Random";
}
class Notebook : public QWidget {
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.fillRect(rect(), QColor("#fcfbf7")); p.setPen(QColor("#dfe6f1"));
        for (int x = 0; x < width(); x += 20) p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 20) p.drawLine(0, y, width(), y);
        p.setPen(QColor("#ec9295")); p.drawLine(7, 0, 7, height());
    }
};
class Paper : public QWidget {
    QColor color_;
public:
    explicit Paper(QColor color = Qt::white) : color_(color) { setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen); p.setBrush(QColor(37, 65, 143, 22)); p.drawRoundedRect(QRectF(4, 5, width() - 5, height() - 6), 3, 4);
        QPolygonF outline; outline << QPointF(1, 2) << QPointF(width() - 6, 1) << QPointF(width() - 3, 6)
            << QPointF(width() - 4, height() - 5) << QPointF(2, height() - 3);
        p.setPen(QPen(Ink, 1.2)); p.setBrush(color_); p.drawPolygon(outline);
    }
};
QLabel *label(const QString &text, int size = 10) {
    auto *widget = new QLabel(text); widget->setFont(hand(size)); widget->setWordWrap(true); return widget;
}
QLabel *heading(QVBoxLayout *layout, const QString &text) {
    auto *widget = label(text, 17); layout->addWidget(widget); return widget;
}
QWidget *field(const QString &text, QWidget *control) {
    auto *widget = new QWidget; auto *layout = new QVBoxLayout(widget); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(5);
    auto *caption = label(text, 10); caption->setBuddy(control); layout->addWidget(caption); layout->addWidget(control);
    control->setAccessibleName(text); return widget;
}
QComboBox *combo(std::initializer_list<std::pair<QString, QString>> entries) {
    auto *widget = new QComboBox;
    for (const auto &entry : entries) widget->addItem(entry.first, entry.second);
    return widget;
}
void addMetricRow(QVBoxLayout *layout, const QString &name, QLabel *&value) {
    auto *row = new QHBoxLayout; row->addWidget(label(name, 10)); row->addStretch();
    value = label("\u2014", 11); row->addWidget(value); layout->addLayout(row);
}
QFrame *separator() { auto *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #d8dfef"); return line; }
}

CaroWindow::CaroWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Caro Arena - Five in a Row"); resize(1280, 870); setMinimumSize(760, 620);
    setStyleSheet(R"(
        QWidget { color: #25418f; }
        QComboBox, QSpinBox { background: #fffef6; border: 1px solid #7a8dbd; border-radius: 4px; padding: 7px; min-height: 20px; }
        QComboBox QAbstractItemView { background: white; selection-background-color: #e5ebff; color: #25418f; }
        QPushButton { border: 1px solid #25418f; border-radius: 6px; background: #f3f4f8; padding: 9px 14px; min-height: 20px; }
        QPushButton:hover { background: #e7edff; }
        QPushButton:pressed { background: #d8e1f9; }
        QPushButton:disabled { color: #9aa7c7; border-color: #bcc5da; }
        QPushButton#primary { background: #25418f; color: white; }
        QPushButton#primary:hover { background: #3757a8; }
        QPushButton#restart { background: #ffd7d2; color: #a4323a; border-color: #ed5659; }
        QListWidget { background: white; border: 0; outline: 0; }
        QListWidget::item { padding: 8px 2px; border-bottom: 1px solid #edf0f6; }
        QLabel#note { color: #8c4b36; background: #fff0d9; padding: 8px; border: 1px dashed #d49a67; }
        QLabel#player { background: white; border: 1px solid #25418f; border-radius: 5px; padding: 10px; }
    )");
    auto *root = new QWidget; auto *rootLayout = new QVBoxLayout(root); rootLayout->setContentsMargins(0, 0, 0, 0); rootLayout->setSpacing(0);
    auto *header = new QFrame; header->setStyleSheet("QFrame { background: white; border-bottom: 2px solid #25418f; }");
    auto *headerLayout = new QHBoxLayout(header); headerLayout->setContentsMargins(26, 12, 26, 12);
    auto *brand = label("\u00d7\u25cb  Caro Arena", 20); brand->setStyleSheet("border: none;"); headerLayout->addWidget(brand); headerLayout->addStretch();
    rootLayout->addWidget(header);
    auto *scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    auto *notebook = new Notebook; notebook->setMinimumWidth(1080);
    auto *workspace = new QHBoxLayout(notebook); workspace->setContentsMargins(38, 32, 38, 32); workspace->setSpacing(30);

    auto *leftColumn = new QVBoxLayout; leftColumn->setSpacing(0); leftColumn->addSpacing(78);
    auto *settings = new Paper(QColor("#fffbbd")); settings->setFixedWidth(250);
    auto *settingsLayout = new QVBoxLayout(settings); settingsLayout->setContentsMargins(22, 22, 22, 24); settingsLayout->setSpacing(13);
    heading(settingsLayout, "01  Game settings");
    mode_ = combo({{"Two players", "human"}, {"Human vs AI", "human-ai"}, {"AI vs AI", "ai-ai"}});
    mode_->setObjectName("gameMode");
    settingsLayout->addWidget(field("Game mode", mode_));
    human_ = combo({{"X - plays first", "X"}, {"O - plays second", "O"}});
    humanSettings_ = field("Your mark", human_); settingsLayout->addWidget(humanSettings_);
    aiSettings_ = new QWidget; auto *aiLayout = new QVBoxLayout(aiSettings_); aiLayout->setContentsMargins(0, 0, 0, 0); aiLayout->setSpacing(12);
    xAgent_ = combo({{"MiniMax + Alpha-Beta", "minimax"}, {"MCTS + UCT", "mcts"}, {"Random", "random"}});
    oAgent_ = combo({{"MCTS + UCT", "mcts"}, {"MiniMax + Alpha-Beta", "minimax"}, {"Random", "random"}});
    xSettings_ = field("Algorithm - X", xAgent_); oSettings_ = field("Algorithm - O", oAgent_);
    aiLayout->addWidget(xSettings_); aiLayout->addWidget(oSettings_); aiLayout->addWidget(separator()); aiLayout->addWidget(label("Algorithm parameters", 12));
    depth_ = new QSpinBox; depth_->setRange(2, 10); depth_->setValue(2);
    candidates_ = new QSpinBox; candidates_->setRange(1, 81); candidates_->setValue(12);
    exploration_ = combo({{"0.5", "0.5"}, {"1.0", "1.0"}, {"1.4", "1.4"}, {"2.0", "2.0"}}); exploration_->setCurrentIndex(2);
    iterations_ = combo({{"100", "100"}, {"300", "300"}, {"500", "500"}}); iterations_->setCurrentIndex(1);
    auto *parameters = new QGridLayout; parameters->setSpacing(10);
    parameters->addWidget(field("Search depth", depth_), 0, 0); parameters->addWidget(field("Candidate limit", candidates_), 0, 1);
    parameters->addWidget(field("Exploration c", exploration_), 1, 0); parameters->addWidget(field("Iterations", iterations_), 1, 1);
    aiLayout->addLayout(parameters); settingsLayout->addWidget(aiSettings_);
    note_ = label("AI is not connected yet. Select Two players to try the game.", 10); note_->setObjectName("note"); settingsLayout->addWidget(note_);
    auto *newGame = new QPushButton("New game  \u2197"); newGame->setObjectName("primary"); newGame->setFont(hand(13)); settingsLayout->addWidget(newGame);
    auto *hint = label("Settings apply when you start a new game.", 9); hint->setAlignment(Qt::AlignCenter); settingsLayout->addWidget(hint);
    settingsLayout->addWidget(separator()); settingsLayout->addWidget(label("HOW TO PLAY", 10));
    settingsLayout->addWidget(label("Connect at least 5 marks horizontally, vertically, or diagonally.", 11));
    settingsLayout->addWidget(label("Blocked ends do not affect a win.", 11));
    settingsLayout->addWidget(label("9 x 9 board                  X plays first", 9));
    leftColumn->addWidget(settings); leftColumn->addStretch(); workspace->addLayout(leftColumn);

    auto *center = new QWidget; center->setMinimumWidth(440); center->setMaximumWidth(620);
    auto *centerLayout = new QVBoxLayout(center); centerLayout->setContentsMargins(0, 0, 0, 0); centerLayout->setSpacing(13);
    auto *players = new QHBoxLayout; players->setSpacing(15);
    xPlayer_ = label("", 11); oPlayer_ = label("", 11); xPlayer_->setObjectName("player"); oPlayer_->setObjectName("player");
    players->addWidget(xPlayer_, 1); auto *vs = label("VS", 11); vs->setAlignment(Qt::AlignCenter); players->addWidget(vs); players->addWidget(oPlayer_, 1);
    centerLayout->addLayout(players);
    auto *boardHeading = new QHBoxLayout; title_ = label("Two players", 11); boardHeading->addWidget(title_); boardHeading->addStretch(); boardHeading->addWidget(label("9 x 9", 10)); centerLayout->addLayout(boardHeading);
    board_ = new BoardWidget; board_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // A square wrapper resizes the drawing surface without stretching the grid.
    auto *boardContainer = new QWidget; auto *boardLayout = new QVBoxLayout(boardContainer); boardLayout->setContentsMargins(0, 0, 0, 0); boardLayout->addWidget(board_);
    boardContainer->setMinimumHeight(440); centerLayout->addWidget(boardContainer);
    auto *statusRow = new QHBoxLayout; status_ = label("", 11); status_->setMinimumHeight(48); statusRow->addWidget(status_, 1); count_ = label("00 / 81", 10); statusRow->addWidget(count_); centerLayout->addLayout(statusRow);
    status_->setObjectName("gameStatus");
    auto *actions = new QHBoxLayout; actions->addStretch(); undo_ = new QPushButton("\u21b6 Undo"); pause_ = new QPushButton("Pause"); auto *restart = new QPushButton("\u21bb Restart"); restart->setObjectName("restart");
    undo_->setObjectName("undo");
    for (auto *button : {undo_, pause_, restart}) { button->setFont(hand(12)); actions->addWidget(button); } actions->addStretch(); centerLayout->addLayout(actions); centerLayout->addStretch();
    workspace->addWidget(center, 1, Qt::AlignTop);

    auto *rightColumn = new QVBoxLayout; rightColumn->setSpacing(0); rightColumn->addSpacing(78);
    auto *activity = new Paper; activity->setFixedWidth(250);
    auto *activityLayout = new QVBoxLayout(activity); activityLayout->setContentsMargins(22, 22, 22, 24); activityLayout->setSpacing(13);
    heading(activityLayout, "02  Game activity"); activityLayout->addWidget(separator());
    auto *stats = new Paper(QColor("#fffbbd")); auto *statsLayout = new QGridLayout(stats); statsLayout->setContentsMargins(12, 10, 12, 13);
    statsLayout->addWidget(label("Moves", 10), 0, 0); statsLayout->addWidget(label("Time elapsed", 10), 0, 1);
    moves_ = label("0", 20); elapsed_ = label("00:00", 20); statsLayout->addWidget(moves_, 1, 0); statsLayout->addWidget(elapsed_, 1, 1); activityLayout->addWidget(stats);
    activityLayout->addSpacing(4); activityLayout->addWidget(label("LATEST AI MOVE", 10));
    addMetricRow(activityLayout, "Thinking time", time_); addMetricRow(activityLayout, "Search depth", metricDepth_); addMetricRow(activityLayout, "Nodes visited", nodes_);
    activityLayout->addWidget(separator());
    auto *historyHeading = new QHBoxLayout; historyHeading->addWidget(label("Move history", 12)); save_ = new QPushButton("Save JSON"); save_->setStyleSheet("border: none; padding: 2px; min-height: 15px;"); save_->setFont(hand(10)); historyHeading->addWidget(save_); activityLayout->addLayout(historyHeading);
    empty_ = label("\u00d7  \u25cb\n\nYour first move awaits.\nMoves will appear here.", 12); empty_->setAlignment(Qt::AlignCenter); empty_->setMinimumHeight(155); activityLayout->addWidget(empty_);
    history_ = new QListWidget; history_->setFont(hand(12)); history_->setMinimumHeight(180); history_->setMaximumHeight(260); history_->setAccessibleName("Move history"); activityLayout->addWidget(history_);
    rightColumn->addWidget(activity); rightColumn->addStretch(); workspace->addLayout(rightColumn);
    scroll->setWidget(notebook); rootLayout->addWidget(scroll, 1); setCentralWidget(root);

    for (auto *widget : {mode_, human_, xAgent_, oAgent_}) connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateSettings(); });
    connect(newGame, &QPushButton::clicked, this, [this] { requestReset(readConfig()); });
    connect(restart, &QPushButton::clicked, this, [this] { requestReset(config_); });
    connect(undo_, &QPushButton::clicked, this, [this] { undo(); });
    connect(pause_, &QPushButton::clicked, this, [this] { paused_ = !paused_; cancelAI(); error_.clear(); render(); if (!paused_) scheduleAI(); });
    connect(save_, &QPushButton::clicked, this, [this] { saveJson(); });
    board_->onMove = [this](caro::Cell cell) { if (!busy_ && !paused_ && !isAI(game_.turn, config_)) place(cell); };
    auto *timer = new QTimer(this); connect(timer, &QTimer::timeout, this, [this] { updateClock(); }); timer->start(1000);
    updateSettings(); startGame(readConfig());
}

CaroWindow::~CaroWindow() { cancelAI(); }
CaroWindow::Config CaroWindow::readConfig() const {
    Config config; config.mode = mode_->currentIndex(); config.human = human_->currentIndex() == 0 ? 'X' : 'O';
    config.xAgent = xAgent_->currentData().toString(); config.oAgent = oAgent_->currentData().toString();
    config.options = {depth_->value(), candidates_->value(), exploration_->currentText().toDouble(), iterations_->currentText().toInt()}; return config;
}
bool CaroWindow::isAI(char player, const Config &config) const { return config.mode == 2 || (config.mode == 1 && player != config.human); }
QString CaroWindow::agent(char player) const { return player == 'X' ? config_.xAgent : config_.oAgent; }
void CaroWindow::updateSettings() {
    const auto config = readConfig(); humanSettings_->setVisible(config.mode == 1); aiSettings_->setVisible(config.mode != 0);
    xSettings_->setVisible(isAI('X', config)); oSettings_->setVisible(isAI('O', config));
    note_->setVisible((isAI('X', config) && !providers_.contains(config.xAgent)) || (isAI('O', config) && !providers_.contains(config.oAgent)));
}
void CaroWindow::registerAI(const QString &name, caro::AiProvider provider) {
    if ((name != "random" && name != "minimax" && name != "mcts") || !provider) throw std::invalid_argument("Invalid AI name or handler");
    providers_.insert(name, std::move(provider)); error_.clear(); updateSettings(); render(); scheduleAI();
}
void CaroWindow::cancelAI() { ++generation_; if (cancellation_) cancellation_->store(true); busy_ = false; }
void CaroWindow::startGame(Config config) {
    cancelAI(); config_ = std::move(config); game_ = {}; clock_.invalidate(); finalElapsed_ = -1; paused_ = false; error_.clear(); render(); scheduleAI();
}
void CaroWindow::requestReset(Config config) {
    if (!game_.moves.empty() && !game_.finished) {
        // Pause the worker before opening the modal dialog's nested event loop.
        cancelAI();
        const auto answer = QMessageBox::question(this, "Start over?", "All moves in the current game will be cleared.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) { render(); scheduleAI(); return; }
    }
    startGame(std::move(config));
}
void CaroWindow::place(caro::Cell cell, std::optional<caro::Metrics> metrics) {
    if (!game_.play(cell, metrics)) return;
    if (!clock_.isValid()) clock_.start();
    if (game_.finished) finalElapsed_ = clock_.elapsed();
    render(); scheduleAI();
}
void CaroWindow::render() {
    board_->setGame(game_, !game_.finished && !busy_ && !paused_ && !isAI(game_.turn, config_));
    for (const auto player : {'X', 'O'}) {
        auto *widget = player == 'X' ? xPlayer_ : oPlayer_;
        const auto color = player == 'X' ? "#25418f" : "#f04d55";
        const QString mark = player == 'X' ? "\u00d7" : "\u25cb";
        const auto name = QString(isAI(player, config_) ? "AI %1" : "Player %1").arg(QChar(player));
        const auto caption = isAI(player, config_) ? algorithmName(agent(player)) : (player == 'X' ? "Plays first" : "Plays second");
        widget->setText(QString("<span style='color:%1; font-size:24px'>%2</span> &nbsp; %3<br><span style='font-size:10px; color:#7787ad'>%4</span>").arg(color, mark, name, caption));
        widget->setStyleSheet(QString("QLabel#player { background: %1; }").arg(!game_.finished && game_.turn == player ? "#fff9d8" : "white"));
    }
    title_->setText(config_.mode == 0 ? "Two players" : config_.mode == 1 ? "Human vs AI" : "AI vs AI");
    QString status;
    if (game_.finished) status = game_.winner ? QString("%1 wins! %2 marks in a row.").arg(QChar(game_.winner)).arg(game_.winning.size()) : "Draw. The board is full.";
    else if (paused_) status = "Game paused.";
    else if (!error_.isEmpty()) status = error_;
    else if (isAI(game_.turn, config_)) status = providers_.contains(agent(game_.turn)) ? QString("AI %1 is thinking...").arg(QChar(game_.turn)) : QString("AI %1's turn. %2 is not connected.").arg(QChar(game_.turn)).arg(algorithmName(agent(game_.turn)));
    else status = QString("%1's turn. Select an empty cell.").arg(QChar(game_.turn));
    status_->setText(status); count_->setText(QString("%1 / 81").arg(game_.moves.size(), 2, 10, QChar('0'))); moves_->setText(QString::number(game_.moves.size()));
    undo_->setEnabled(!game_.moves.empty() && config_.mode != 2); save_->setEnabled(!game_.moves.empty()); pause_->setVisible(config_.mode == 2 && !game_.finished); pause_->setText(paused_ ? "Resume" : "Pause");
    empty_->setVisible(game_.moves.empty()); history_->setVisible(!game_.moves.empty()); history_->clear();
    time_->setText("\u2014"); metricDepth_->setText("\u2014"); nodes_->setText("\u2014"); bool foundMetrics = false;
    for (auto it = game_.moves.rbegin(); it != game_.moves.rend(); ++it) {
        const int moveNumber = int(game_.moves.size() - std::distance(game_.moves.rbegin(), it));
        const auto meta = it->metrics ? QString::number(it->metrics->timeMs, 'f', 1) + " ms" : "Human";
        auto *item = new QListWidgetItem(QString("%1    %2    %3    %4").arg(moveNumber, 2, 10, QChar('0')).arg(QChar(it->player)).arg(coordinate(it->cell)).arg(meta), history_);
        item->setForeground(QColor(it->player == 'X' ? "#25418f" : "#e45057"));
        if (!foundMetrics && it->metrics) {
            foundMetrics = true; const auto &metrics = *it->metrics;
            time_->setText(QString::number(metrics.timeMs, 'f', 1) + " ms");
            if (metrics.depth) metricDepth_->setText(QString::number(*metrics.depth));
            if (metrics.nodes) nodes_->setText(QLocale(QLocale::English).toString(qlonglong(*metrics.nodes)));
        }
    }
    updateClock();
}
void CaroWindow::scheduleAI() {
    if (game_.finished || busy_ || paused_ || !error_.isEmpty() || !isAI(game_.turn, config_) || !providers_.contains(agent(game_.turn))) return;
    busy_ = true; cancellation_ = std::make_shared<std::atomic_bool>(false);
    if (!clock_.isValid()) clock_.start();
    const auto token = generation_; const auto provider = providers_.value(agent(game_.turn));
    const caro::AiRequest request{game_.board, game_.turn, config_.options, cancellation_};
    render();
    struct Outcome { caro::AiResponse response; double time = 0; };
    auto *watcher = new QFutureWatcher<Outcome>(this);
    connect(watcher, &QFutureWatcher<Outcome>::finished, this, [this, watcher, token, request] {
        const auto outcome = watcher->result(); watcher->deleteLater();
        if (token != generation_ || request.cancelled->load()) return;
        busy_ = false;
        const auto &response = outcome.response;
        if (!response.error.empty()) { error_ = "AI error: " + QString::fromStdString(response.error); render(); return; }
        if (!caro::valid(response.cell) || game_.board[response.cell.row][response.cell.col]) { error_ = "AI returned an invalid move."; render(); return; }
        caro::Metrics metrics{outcome.time, response.depth, response.nodes}; place(response.cell, metrics);
    });
    watcher->setFuture(QtConcurrent::run([provider, request] {
        QElapsedTimer timer; timer.start(); Outcome outcome;
        try { outcome.response = provider(request); }
        catch (const std::exception &exception) { outcome.response.error = exception.what(); }
        catch (...) { outcome.response.error = "Unknown provider error."; }
        outcome.time = timer.nsecsElapsed() / 1000000.0; return outcome;
    }));
}
void CaroWindow::undo() {
    cancelAI(); paused_ = false; error_.clear(); finalElapsed_ = -1;
    do { if (!game_.undo()) break; } while (config_.mode == 1 && game_.turn != config_.human && !game_.moves.empty());
    if (game_.moves.empty()) clock_.invalidate();
    render(); scheduleAI();
}
void CaroWindow::updateClock() {
    const auto seconds = (clock_.isValid() ? (finalElapsed_ >= 0 ? finalElapsed_ : clock_.elapsed()) : 0) / 1000;
    elapsed_->setText(QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0')));
}
void CaroWindow::saveJson() {
    // Snapshot before the native file dialog opens its event loop.
    const auto snapshot = game_; const auto settings = config_;
    const auto path = QFileDialog::getSaveFileName(this, "Save game history", "caro-game.json", "JSON files (*.json)");
    if (path.isEmpty()) return;
    QJsonArray moves, board, winning;
    for (const auto &row : snapshot.board) { QJsonArray cells; for (char cell : row) cells.append(cell ? QJsonValue(QString(QChar(cell))) : QJsonValue(QJsonValue::Null)); board.append(cells); }
    for (const auto &move : snapshot.moves) {
        QJsonObject json{{"row", move.cell.row}, {"col", move.cell.col}, {"player", QString(QChar(move.player))}, {"coordinate", coordinate(move.cell)}};
        if (move.metrics) {
            QJsonObject metrics{{"timeMs", move.metrics->timeMs}};
            if (move.metrics->depth) metrics["depth"] = *move.metrics->depth;
            if (move.metrics->nodes) metrics["nodes"] = double(*move.metrics->nodes);
            json["metrics"] = metrics;
        }
        moves.append(json);
    }
    for (auto cell : snapshot.winning) winning.append(QJsonObject{{"row", cell.row}, {"col", cell.col}});
    QJsonObject options{{"depth", settings.options.depth}, {"candidateLimit", settings.options.candidateLimit}, {"c", settings.options.c}, {"iterations", settings.options.iterations}};
    QJsonObject config{{"mode", settings.mode == 0 ? "human" : settings.mode == 1 ? "human-ai" : "ai-ai"}, {"humanPiece", QString(QChar(settings.human))}, {"xAgent", settings.xAgent}, {"oAgent", settings.oAgent}, {"options", options}};
    QJsonObject json{{"boardSize", 9}, {"board", board}, {"config", config}, {"moves", moves}, {"finished", snapshot.finished}, {"winner", snapshot.winner ? QJsonValue(QString(QChar(snapshot.winner))) : QJsonValue(QJsonValue::Null)}, {"winningLine", winning}};
    QSaveFile file(path); const auto data = QJsonDocument(json).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) QMessageBox::warning(this, "Save failed", "Could not save the game history.\n" + file.errorString());
}
