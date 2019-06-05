#include <QImage>
#include <QColor>
#include <QPainter>

#include <vector>

#include "screen.h"

using std::vector;

Screen::Screen(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_width(160),
      m_height(144)
{

}

void Screen::paint(QPainter *painter)
{
    QImage canvas(m_width, m_height, QImage::Format_RGBA8888);

    vector<GPU::RGB> rgb = m_console.getRGB();
    for (size_t i = 0; i < rgb.size(); i++) {
        const GPU::RGB & color = rgb.at(i);

        int x = i % m_width;
        int y = i / m_width;

        canvas.setPixel(x, y, qRgba(color.red, color.green, color.blue, color.alpha));
    }

    painter->drawImage(20, 20, canvas);
}
