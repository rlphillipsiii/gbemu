#include <QImage>
#include <QColor>
#include <QPainter>
#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGVertexColorMaterial>
#include <QThread>
#include <QString>

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
using std::shared_ptr;
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
      m_width(LCD_SCREEN_WIDTH),
      m_height(LCD_SCREEN_HEIGHT),
      m_canvas(m_width, m_height, QImage::Format_RGBA8888),
      m_console(GameBoyInterface::Instance()),
      m_stopped(false)
{
    assert(m_console);

    setFlag(ItemHasContents, true);

    QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

    m_timer.setInterval(10);
    m_timer.start();

    QStringList args = QCoreApplication::arguments();

    string filename = (args.size() < 2) ?
        Configuration::getString(ConfigKey::ROM) : args.at(1).toStdString();

    if (m_console) {
        m_console->load(filename);
        m_console->start();
    }
}

Screen::~Screen()
{
    stop();
}

void Screen::stop()
{
    if (m_stopped) { return; }

    m_timer.stop();

    if (m_console) { m_console->stop(); }
}

void Screen::keyPressEvent(QKeyEvent *event)
{
    auto iterator = BUTTON_MAP.find(event->key());
    if (BUTTON_MAP.end() != iterator) {
        m_console->setButton(iterator->second);
    }
}

void Screen::keyReleaseEvent(QKeyEvent *event)
{
    auto iterator = BUTTON_MAP.find(event->key());
    if (BUTTON_MAP.end() != iterator) {
        m_console->clrButton(iterator->second);
    }
}

void Screen::paint(QPainter *painter)
{
    painter->drawImage(0, 0, m_canvas.scaled(width(), height()));
}

void Screen::onTimeout()
{
    ColorArray rgb = m_console->getRGB();
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

void Screen::updateCanvas(uint8_t x, uint8_t y, GB::RGB pixel)
{
    QRgb rgb = qRgba(pixel.red, pixel.green, pixel.blue, pixel.alpha);
    m_canvas.setPixel(x, y, rgb);
}

void Screen::renderCanvas()
{

}
