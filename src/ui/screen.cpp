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
      m_width(GameBoyInterface::WIDTH),
      m_height(GameBoyInterface::HEIGHT),
      m_canvas(m_width, m_height, QImage::Format_RGBA8888),
      m_console(GameBoyInterface::Instance()),
      m_stopped(false)
{
    assert(m_console);

    setFlag(ItemHasContents, true);

    QObject::connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

    QStringList args = QCoreApplication::arguments();

    string filename = (args.size() < 2) ?
        Configuration::getString(ConfigKey::ROM) : args.at(1).toStdString();

    m_rom = QString::fromStdString(filename);
    if (m_console) {
        m_console->load(filename);
        m_console->start();
    }

    m_timer.setInterval(REFRESH_TIMEOUT);
    m_timer.start();
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

void Screen::setLinkMaster(bool master)
{
    Configuration::updateBool(ConfigKey::LINK_MASTER, master);
}
bool Screen::getLinkMaster() const
{
    return Configuration::getBool(ConfigKey::LINK_MASTER);
}

void Screen::setLinkType(ScreenTypes::ConsoleLinkType type)
{
    Configuration::updateInt(ConfigKey::LINK_TYPE, type);
}
ScreenTypes::ConsoleLinkType Screen::getLinkType() const
{
    return Configuration::getEnum<ScreenTypes::ConsoleLinkType>(ConfigKey::LINK_TYPE);
}

void Screen::setSpeed(ScreenTypes::EmulationSpeed speed)
{
    Configuration::updateInt(ConfigKey::SPEED, speed);
}
ScreenTypes::EmulationSpeed Screen::getSpeed() const
{
    return Configuration::getEnum<ScreenTypes::EmulationSpeed>(ConfigKey::SPEED);
}

void Screen::setMode(ScreenTypes::EmulationMode mode)
{
    Configuration::updateInt(ConfigKey::EMU_MODE, mode);
}
ScreenTypes::EmulationMode Screen::getMode() const
{
    return Configuration::getEnum<ScreenTypes::EmulationMode>(ConfigKey::EMU_MODE);
}

void Screen::setLinkEnable(bool enable)
{
    Configuration::updateBool(ConfigKey::LINK_ENABLE, enable);
}
bool Screen::getLinkEnable() const
{
    return Configuration::getBool(ConfigKey::LINK_ENABLE);
}

void Screen::setROM(QString rom)
{
    stop();

    // TODO: probably shouldn't reconstruct the whole object here
    m_console = shared_ptr<GameBoyInterface>(GameBoyInterface::Instance());

    assert(m_console);

    string filename = QUrl(rom).toLocalFile().toStdString();
    Configuration::updateString(ConfigKey::ROM, filename);

    m_console->load(filename);
    m_console->start();

    m_timer.start();

    m_stopped = false;
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
