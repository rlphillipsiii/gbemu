#include <QImage>
#include <QColor>
#include <QPainter>

#include <vector>

#include "screen.h"

using std::vector;

Screen::Screen(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_width(LCD_SCREEN_WIDTH),
      m_height(LCD_SCREEN_HEIGHT),
      m_canvas(m_width, m_height, QImage::Format_RGBA8888)
{

}

void Screen::paint(QPainter *painter)
{
    vector<GPU::RGB> rgb = m_console.getRGB();
    for (size_t i = 0; i < rgb.size(); i++) {
        const GPU::RGB & color = rgb.at(i);

        int x = i % m_width;
        int y = i / m_width;

        m_canvas.setPixel(x, y, qRgba(color.red, color.green, color.blue, color.alpha));
    }

    painter->drawImage(0, 0, m_canvas.scaled(width(), height()));
}
