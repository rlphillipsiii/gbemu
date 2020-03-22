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
    Q_PROPERTY(QString rom READ getROM WRITE setROM NOTIFY romChanged)

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen();

    void paint(QPainter *painter) override;

    void updateCanvas(uint8_t x, uint8_t y, GB::RGB pixel) override;
    void renderCanvas() override;

    QString getROM() const { return m_rom; }
    void setROM(QString rom);

public slots:
    void onTimeout();
    void stop();

signals:
    void romChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static constexpr int REFRESH_TIMEOUT = 20;

    static const std::unordered_map<int, GameBoyInterface::JoyPadButton> BUTTON_MAP;

    Screen(const Screen &) = delete;
    Screen & operator=(const Screen &) = delete;

    int m_width;
    int m_height;

    QImage m_canvas;

    std::shared_ptr<GameBoyInterface> m_console;

    bool m_stopped;

    QTimer m_timer;

    QString m_rom;
};
#endif
