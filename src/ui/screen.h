#ifndef _SCREEN_H
#define _SCREEN_H

#include <QtQuick/QQuickPaintedItem>
#include <QObject>
#include <QVector>

#include <cstdint>

#include "gameboy.h"

class Screen : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen() = default;

    Q_INVOKABLE inline int length() const      { return int(m_pixels.size());    }
    Q_INVOKABLE inline int at(int index) const { return int(m_pixels.at(index)); }

    void paint(QPainter *painter) override;

private:    
    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QVector<uint8_t> m_pixels;

    GameBoy m_console;
};

#endif
