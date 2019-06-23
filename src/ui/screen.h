#ifndef _SCREEN_H
#define _SCREEN_H

#include <QtQuick/QQuickPaintedItem>
#include <QObject>
#include <QImage>
#include <QTimer>

#include <cstdint>
#include <memory>

class GameBoyInterface;

class Screen : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen();

    void paint(QPainter *painter) override;
    // QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

public slots:
    void onTimeout();
    void stop();
    
private:    
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
