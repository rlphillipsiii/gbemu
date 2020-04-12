#ifndef _CLOCK_INTERFACE_H
#define _CLOCK_INTERFACE_H

#include <cstdint>

class ClockInterface {
public:
    virtual void tick(uint8_t cycles) = 0;
};

#endif
