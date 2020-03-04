#ifndef _SCREEN_H
#define _SCREEN_H

#include <QtQuick/QQuickPaintedItem>
#include <QObject>
#include <QImage>
#include <QTimer>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "gameboyinterface.h"
#include "canvasinterface.h"

class Screen : public QQuickPaintedItem, public CanvasInterface {
    Q_OBJECT

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen();

    void paint(QPainter *painter) override;
    // QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

    void updateCanvas(uint8_t x, uint8_t y, GB::RGB pixel);
    void renderCanvas();

public slots:
    void onTimeout();
    void stop();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static const std::unordered_map<int, GameBoyInterface::JoyPadButton> BUTTON_MAP;

    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QImage m_canvas;

    std::shared_ptr<GameBoyInterface> m_console;

    bool m_stopped;

    QTimer m_timer;
};
#endif
