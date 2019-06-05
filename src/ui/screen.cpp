#include <vector>

#include "screen.h"

using std::vector;

Screen::Screen(QObject *parent)
    : QObject(parent),
      m_width(160),
      m_height(144)
{
    // RGB8888 = 4 bytes per pixel
    m_pixels.resize(4 * m_width * m_height);

    vector<GPU::RGB> rgb = m_console.getRGB();

    int index = 0;
    for (int64_t i = 0; i < m_pixels.size(); i += 4) {
        GPU::RGB & pixel = rgb[index++];

        m_pixels[i]     = pixel.red;
        m_pixels[i + 1] = pixel.green;
        m_pixels[i + 2] = pixel.blue;
        
        // Initialize all of the alpha values to 255
        m_pixels[i + 3] = 0xFF;
    }
}
