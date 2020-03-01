#ifndef _VIDEO_RAM_H
#define _VIDEO_RAM_H

#include <vector>
#include <cstdint>

#include "memoryregion.h"

class MemoryController;

class VideoRam : public MemoryRegion {
public:
    explicit VideoRam(MemoryController & parent, uint16_t size, uint16_t offset);

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

private:
    static const uint16_t BANK_SELECT_ADDRESS;

    MemoryController & m_parent;

    std::vector<uint8_t> m_bank;
};

#endif
