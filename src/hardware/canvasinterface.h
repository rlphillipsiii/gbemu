#ifndef _CANVAS_INTERFACE_H
#define _CANVAS_INTERFACE_H

#include <cstdint>

#include "gbrgb.h"

class CanvasInterface {
public:
    virtual void updateCanvas(uint8_t x, uint8_t y, GB::RGB colors) = 0;
    virtual void renderCanvas() = 0;
};

#endif
