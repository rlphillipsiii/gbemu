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
#include <string>

#include "consolesubscriber.h"

class Screen : public QQuickPaintedItem, public ConsoleSubscriber {
    Q_OBJECT

public:
    explicit Screen(QQuickItem *parent = nullptr);
    ~Screen();

    void paint(QPainter *painter) override;

    static void registerQML()
    {
        qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");
    }

    Q_INVOKABLE void start();

public slots:
    void onTimeout();
    Q_INVOKABLE void stop();
    void onPause();
    void onResume();

signals:
    void notify();
    void frameRateUpdated(int rate);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static constexpr const int REFRESH_TIMEOUT = 33;

    static const std::unordered_map<int, GameBoyInterface::JoyPadButton> BUTTON_MAP;

    Screen(const Screen &) = delete;
    Screen(const Screen &&) = delete;
    Screen(Screen &&) = delete;
    Screen & operator=(const Screen &) = delete;

    std::mutex m_lock;

    int m_width;
    int m_height;

    QImage m_canvas;
    QTimer m_timer;

    bool m_stopped;
    bool m_notify;
};

#endif
