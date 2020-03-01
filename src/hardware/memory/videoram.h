#ifndef _VIDEO_RAM_H
#define _VIDEO_RAM_H

#include <vector>
#include <cstdint>

#include "memoryregion.h"

class MemoryController;

class VideoRam : public MemoryRegion {
public:
    explicit VideoRam(MemoryController & parent, uint16_t size, uint16_t offset);

    bool isBankingActive(uint16_t) const override;

private:
    static const uint8_t BANK_COUNT;
    static const uint16_t BANK_SELECT_ADDRESS;
};

#endif
