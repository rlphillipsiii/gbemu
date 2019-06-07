#ifndef _SCREEN_H
#define _SCREEN_H

#include <QtQuick/QQuickPaintedItem>
#include <QObject>
#include <QImage>

#include <cstdint>

#include "gameboy.h"

class Screen : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen() = default;

    void paint(QPainter *painter) override;

private:    
    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QImage m_canvas;

    GameBoy m_console;
};

#endif
