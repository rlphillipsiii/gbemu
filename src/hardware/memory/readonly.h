#ifndef _READ_ONLY_H
#define _READ_ONLY_H

#include <cstdint>

#include "memoryregion.h"

class MemoryController;

class ReadOnly : public MemoryRegion {
public:
    explicit ReadOnly(MemoryController & parent, uint16_t size, uint16_t offset);
    ~ReadOnly() = default;

    inline bool isWritable() const override { return false; }

    void write(uint16_t address, uint8_t value) override;
};

#endif
