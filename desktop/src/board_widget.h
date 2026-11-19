#pragma once
#include "game.h"
#include <QWidget>
#include <functional>

class BoardWidget : public QWidget {
public:
    explicit BoardWidget(QWidget *parent = nullptr);
    void setGame(const caro::Game &game, bool playable);
    std::function<void(caro::Cell)> onMove;
    QSize sizeHint() const override { return {510, 510}; }
    QSize minimumSizeHint() const override { return {340, 340}; }
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void resizeEvent(QResizeEvent *) override;
private:
    QRectF gridRect() const;
    caro::Cell cellAt(QPointF pos) const;
    caro::Game game_;
    bool playable_ = true;
    caro::Cell hover_, focus_{4, 4};
};
