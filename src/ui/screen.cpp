#include <QImage>
#include <QColor>
#include <QPainter>
#include <QThread>
#include <QString>
#include <QUrl>

#include <cassert>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <string>

#include "screen.h"
#include "logging.h"
#include "gameboyinterface.h"
#include "configuration.h"

using std::vector;
using std::unique_ptr;
using std::unordered_map;
using std::string;

const unordered_map<int, GameBoyInterface::JoyPadButton> Screen::BUTTON_MAP = {
    { Qt::Key_Left,  GameBoyInterface::JOYPAD_LEFT   },
    { Qt::Key_Right, GameBoyInterface::JOYPAD_RIGHT  },
    { Qt::Key_Down,  GameBoyInterface::JOYPAD_DOWN   },
    { Qt::Key_Up,    GameBoyInterface::JOYPAD_UP     },
    { Qt::Key_A,     GameBoyInterface::JOYPAD_A      },
    { Qt::Key_B,     GameBoyInterface::JOYPAD_B      },
    { Qt::Key_Space, GameBoyInterface::JOYPAD_START  },
    { Qt::Key_F,     GameBoyInterface::JOYPAD_SELECT },
};

Screen::Screen(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      ConsoleSubscriber(),
      m_width(GameBoyInterface::WIDTH),
      m_height(GameBoyInterface::HEIGHT),
      m_canvas(m_width, m_height, QImage::Format_RGBA8888),
      m_stopped(true)
{
    setFlag(ItemHasContents, true);

    QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

Screen::~Screen()
{
    stop();
}

void Screen::start()
{
    m_timer.setInterval(REFRESH_TIMEOUT);
    m_timer.start();

    m_stopped = false;
}

void Screen::stop()
{
    if (m_stopped) { return; }

    m_stopped = true;
    m_timer.stop();
}

void Screen::keyPressEvent(QKeyEvent *event)
{
    auto & console = m_console.get();
    if (!console) { return; }

    auto iterator = BUTTON_MAP.find(event->key());
    if (BUTTON_MAP.end() == iterator) {
        event->ignore();
        return;
    }

    console->setButton(iterator->second);
}

void Screen::keyReleaseEvent(QKeyEvent *event)
{
    auto & console = m_console.get();
    if (!console) { return; }

    auto iterator = BUTTON_MAP.find(event->key());
    if (BUTTON_MAP.end() == iterator) {
        event->ignore();
        return;
    }

    console->clrButton(iterator->second);
}

void Screen::paint(QPainter *painter)
{
    painter->drawImage(0, 0, m_canvas.scaled(width(), height()));
}

void Screen::onPause()
{
    stop();
}

void Screen::onResume()
{
    start();
}

void Screen::onTimeout()
{
    auto & console = m_console.get();
    if (!console) { return; }

    ColorArray rgb = console->getRGB();
    if (rgb.empty()) { return; }

    for (size_t i = 0; i < rgb.size(); i++) {
        const GB::RGB & color = rgb.at(i);

        int x = i % m_width;
        int y = i / m_width;

        QRgb rgb = qRgba(color.red, color.green, color.blue, color.alpha);
        m_canvas.setPixel(x, y, rgb);
    }

    update();
}
