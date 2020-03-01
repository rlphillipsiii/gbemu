#ifndef _WORKING_RAM_H
#define _WORKING_RAM_H

#include <vector>
#include <cstdint>

#include "memoryregion.h"

class WorkingRam : public MemoryRegion {
public:
    explicit WorkingRam(uint16_t size, uint16_t offset);
    ~WorkingRam() = default;

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

    bool isAddressed(uint16_t address) const override;

private:
    uint16_t m_shadowOffset;

    std::vector<uint8_t> m_shadow;

    bool isShadowAddressed(uint16_t address) const;
};

#endif
