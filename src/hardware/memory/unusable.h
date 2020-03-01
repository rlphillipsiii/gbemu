#ifndef _UNUSABLE_H
#define _UNUSABLE_H

#include <cstdint>

#include "memoryregion.h"

class Unusable : public MemoryRegion {
public:
    explicit Unusable(uint16_t size, uint16_t offset)
        : MemoryRegion(size, offset), m_dummy(0xFF) { m_memory.resize(0); }
    ~Unusable() = default;

    void write(uint16_t, uint8_t) override { }
    uint8_t & read(uint16_t) override { return m_dummy; }

private:
    uint8_t m_dummy;
};

#endif
