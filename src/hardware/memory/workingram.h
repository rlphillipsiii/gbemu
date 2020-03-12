#ifndef _WORKING_RAM_H
#define _WORKING_RAM_H

#include <vector>
#include <cstdint>

#include "memoryregion.h"

class MemoryController;

class WorkingRam : public MemoryRegion {
public:
    explicit WorkingRam(MemoryController & parent, uint16_t size, uint16_t offset);
    ~WorkingRam() = default;

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

    bool isAddressed(uint16_t address) const override;

private:
    static const uint8_t BANK_COUNT;
    static const uint16_t BANK_SELECT_ADDRESS;

    bool isShadowAddressed(uint16_t address) const;

    std::vector<uint8_t> & bank(uint16_t index);
};

#endif
