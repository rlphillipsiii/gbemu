#ifndef _READ_ONLY_H
#define _READ_ONLY_H

#include <cstdint>

#include "memoryregion.h"

class MemoryController;

class ReadOnly : public MemoryRegion {
public:
    explicit ReadOnly(MemoryController & parent, uint16_t size, uint16_t offset);
    virtual ~ReadOnly() = default;

    inline bool isWritable() const override { return false; }
    inline void reset() override { }

    void write(uint16_t address, uint8_t value) override;
};

class Bios : public ReadOnly {
public:
    explicit Bios(MemoryController & parent, uint16_t size, uint16_t offset)
        : ReadOnly(parent, size, offset) { }
    ~Bios() = default;

    bool isAddressed(uint16_t address) const override
    {
        // CGB Bios code is made up of two non-contiguous regions, so we need to
        // make sure that we aren't in between the two regions.
        if ((address >= 0x0100) && (address < 0x0200)) { return false; }
        return ReadOnly::isAddressed(address);
    }
};

#endif
