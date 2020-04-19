#ifndef _MAPPED_IO_H
#define _MAPPED_IO_H

#include <cstdint>
#include <functional>
#include <vector>
#include <memory>

#include "memoryregion.h"
#include "consolelink.h"

class GPU;

class MappedIO : public MemoryRegion {
public:
    explicit MappedIO(
        MemoryController & memory,
        GPU & gpu,
        std::unique_ptr<ConsoleLink> & link,
        uint16_t address,
        uint16_t offset);
    ~MappedIO() = default;

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

    void reset() override { }

    inline void writeBytes(uint16_t ptr, uint8_t value, uint8_t mask)
        { writeBytes(MemoryRegion::read(ptr), value, mask); }
    inline void writeBytes(uint8_t & reg, uint8_t value, uint8_t mask)
        { reg = ((reg & ~mask) | (value & mask)); }

    inline uint8_t resetValue() const override { return 0xFF; }

    inline bool isRtcResetRequested() const { return m_rtcReset; }

    inline void clearRtcReset() { m_rtcReset = false; }

private:
    GPU & m_gpu;
    std::unique_ptr<ConsoleLink> & m_link;

    bool m_rtcReset;
};

#endif
