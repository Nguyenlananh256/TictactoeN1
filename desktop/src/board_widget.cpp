#include "board_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <algorithm>

namespace {
const QColor Ink("#25418f"), Red("#f04d55");
QPointF mousePosition(const QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position();
#else
    return event->localPos();
#endif
}
}
BoardWidget::BoardWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(340, 340); setMouseTracking(true); setFocusPolicy(Qt::StrongFocus);
    setAccessibleName("Caro board: 9 rows and 9 columns");
}
void BoardWidget::setGame(const caro::Game &game, bool playable) {
    game_ = game; playable_ = playable; setCursor(playable ? Qt::PointingHandCursor : Qt::ArrowCursor); update();
}
QRectF BoardWidget::gridRect() const {
    const double size = std::min(width() - 42.0, height() - 42.0);
    return {(width() - size) / 2.0 + 6, (height() - size) / 2.0 + 6, size, size};
}
caro::Cell BoardWidget::cellAt(QPointF pos) const {
    const auto grid = gridRect();
    if (!grid.contains(pos) || pos.x() >= grid.right() || pos.y() >= grid.bottom()) return {};
    return {int((pos.y() - grid.top()) / (grid.height() / 9)), int((pos.x() - grid.left()) / (grid.width() / 9))};
}
void BoardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Ink, 1.3)); p.setBrush(Qt::white);
    p.drawRoundedRect(QRectF(2, 2, width() - 5, height() - 5), 3, 5);
    const auto grid = gridRect(); const double size = grid.width() / 9;
    const auto last = game_.moves.empty() ? caro::Cell{} : game_.moves.back().cell;
    for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) {
        QRectF rect(grid.left() + c * size, grid.top() + r * size, size, size);
        const caro::Cell cell{r, c};
        if (cell == last) p.fillRect(rect, QColor("#fff5cb"));
        if (playable_ && cell == hover_ && !game_.board[r][c]) p.fillRect(rect, QColor("#edf2ff"));
        if (std::find(game_.winning.begin(), game_.winning.end(), cell) != game_.winning.end()) p.fillRect(rect, QColor("#dff1d5"));
        const auto inset = rect.adjusted(size * .23, size * .23, -size * .23, -size * .23);
        if (game_.board[r][c] == 'X') {
            p.setPen(QPen(Ink, 2.5, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(inset.topLeft() + QPointF(0, 1), inset.bottomRight());
            p.drawLine(inset.topRight(), inset.bottomLeft() + QPointF(1, 0));
        } else if (game_.board[r][c] == 'O') {
            p.setPen(QPen(Red, 2.5, Qt::SolidLine, Qt::RoundCap)); p.setBrush(Qt::NoBrush);
            QPainterPath path; path.moveTo(inset.center().x(), inset.top());
            path.cubicTo(inset.right() + 2, inset.top(), inset.right() + 2, inset.bottom(), inset.center().x(), inset.bottom());
            path.cubicTo(inset.left() - 2, inset.bottom() + 1, inset.left() - 1, inset.top() - 1, inset.center().x(), inset.top());
            p.drawPath(path);
        }
        if (cell == last) { p.setPen(QPen(QColor("#dfb43c"), 1.6)); p.setBrush(Qt::NoBrush); p.drawRect(rect.adjusted(2, 2, -2, -2)); }
        if (hasFocus() && cell == focus_) { p.setPen(QPen(Ink, 1.5, Qt::DashLine)); p.setBrush(Qt::NoBrush); p.drawRect(rect.adjusted(4, 4, -4, -4)); }
    }
    p.setPen(QPen(QColor("#8294bf"), .8));
    for (int i = 0; i <= 9; ++i) {
        const double offset = (i % 2) * .6;
        p.drawLine(QPointF(grid.left() + i * size, grid.top()), QPointF(grid.left() + i * size + offset, grid.bottom()));
        p.drawLine(QPointF(grid.left(), grid.top() + i * size), QPointF(grid.right(), grid.top() + i * size + offset));
    }
    p.setFont(QFont("Patrick Hand", 11)); p.setPen(QColor("#7b8fbd"));
    for (int i = 0; i < 9; ++i) {
        p.drawText(QRectF(grid.left() + i * size, grid.top() - 23, size, 20), Qt::AlignCenter, QString(QChar('A' + i)));
        p.drawText(QRectF(grid.left() - 23, grid.top() + i * size, 20, size), Qt::AlignCenter, QString::number(i + 1));
    }
}
void BoardWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;
    const auto cell = cellAt(mousePosition(event));
    if (caro::valid(cell)) { focus_ = cell; setFocus(); update(); if (playable_ && onMove) onMove(cell); }
}
void BoardWidget::mouseMoveEvent(QMouseEvent *event) { hover_ = cellAt(mousePosition(event)); update(); }
void BoardWidget::leaveEvent(QEvent *) { hover_ = {}; update(); }
void BoardWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (height() != width()) setFixedHeight(width());
}
void BoardWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Up: focus_.row = std::max(0, focus_.row - 1); break;
    case Qt::Key_Down: focus_.row = std::min(8, focus_.row + 1); break;
    case Qt::Key_Left: focus_.col = std::max(0, focus_.col - 1); break;
    case Qt::Key_Right: focus_.col = std::min(8, focus_.col + 1); break;
    case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Space: if (playable_ && onMove) onMove(focus_); break;
    default: QWidget::keyPressEvent(event); return;
    }
    update();
}
