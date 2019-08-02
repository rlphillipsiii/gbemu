#include <QImage>
#include <QColor>
#include <QPainter>
#include <QSGGeometryNode>
#include <QSGGeometry>
#include <QSGVertexColorMaterial>
#include <QThread>

#include <cassert>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "screen.h"
#include "logging.h"
#include "gameboyinterface.h"

using std::vector;
using std::shared_ptr;
using std::unordered_map;

const unordered_map<int, GameBoyInterface::JoyPadButton> Screen::BUTTON_MAP = {
    { Qt::Key_Left,  GameBoyInterface::JOYPAD_LEFT  },
    { Qt::Key_Right, GameBoyInterface::JOYPAD_RIGHT },
    { Qt::Key_Down,  GameBoyInterface::JOYPAD_DOWN  },
    { Qt::Key_Up,    GameBoyInterface::JOYPAD_UP    },
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

    m_timer.setInterval(30);
    m_timer.start();

    m_console->load("rom.gb");
    m_console->start();
}

Screen::~Screen()
{
    stop();
}

void Screen::stop()
{
    if (m_stopped) { return; }

    m_timer.stop();

    m_console->stop();
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
    while (!m_console->idle());
    
    ColorArray rgb = m_console->getRGB();
    m_console->advance();
    
    for (size_t i = 0; i < rgb.size(); i++) {
        shared_ptr<GB::RGB> color = rgb.at(i);

        int x = i % m_width;
        int y = i / m_width;

#ifdef DEBUG
        if (!color) {
            LOG("Invalid color at (%d, %d)\n", x, y);
            assert(0);
        }
#endif

        m_canvas.setPixel(x, y, qRgba(color->red, color->green, color->blue, color->alpha));
    }

    painter->drawImage(0, 0, m_canvas.scaled(width(), height()));
}

void Screen::onTimeout()
{
    update();
}

#if 0
QSGNode *Screen::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGGeometryNode *root = static_cast<QSGGeometryNode*>(oldNode);
    
    if (!root) {
        root = new QSGGeometryNode();
        root->setFlag(QSGNode::OwnsMaterial, true);
        root->setFlag(QSGNode::OwnsGeometry, true);
    }

    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), m_width * m_height);
    QSGGeometry::ColoredPoint2D *points = geometry->vertexDataAsColoredPoint2D();

    vector<GPU::RGB> rgb = m_console.getRGB();
    for (size_t i = 0; i < rgb.size(); i++) {
        const GPU::RGB & color = rgb.at(i);

        int x = i % m_width;
        int y = i / m_width;

        points[i].set(x, y, color.red, color.green, color.blue, color.alpha);
    }

    root->setGeometry(geometry);

    QSGVertexColorMaterial *material = new QSGVertexColorMaterial();
    root->setMaterial(material);
 
    return root;
}
#endif
