#ifndef _SERIALIZABLE_H
#define _SERIALIZABLE_H

#include <vector>
#include <cstdint>

class Serializable {
public:
    enum SerialType {
        SERIAL_MMC    = 0x00,
        SERIAL_CPU    = 0x01,
        SERIAL_JOYPAD = 0x02,
        SERIAL_TIMER  = 0x03,
        SERIAL_GPU    = 0x04,
    };

    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool recall(std::vector<uint8_t>::iterator & stream) = 0;
};

#endif
