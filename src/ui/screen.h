#ifndef _SCREEN_H
#define _SCREEN_H

#include <QObject>
#include <QVector>

#include <cstdint>

#include "gameboy.h"

class Screen : public QObject {
    Q_OBJECT

public:
    explicit Screen(QObject *parent = nullptr);
    ~Screen() = default;

    Q_INVOKABLE QVector<uint8_t> *pixels() { return &m_pixels; }

    Q_INVOKABLE inline int length() const      { return int(m_pixels.size());    }
    Q_INVOKABLE inline int at(int index) const { return int(m_pixels.at(index)); }

private:    
    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QVector<uint8_t> m_pixels;

    GameBoy m_console;
};

#endif
