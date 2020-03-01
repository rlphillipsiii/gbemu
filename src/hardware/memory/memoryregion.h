#ifndef _MEMORY_REGION_H
#define _MEMORY_REGION_H

#include <vector>
#include <algorithm>
#include <cstdint>

class MemoryRegion {
public:
    explicit MemoryRegion(uint16_t size, uint16_t offset);
    virtual ~MemoryRegion() = default;

    MemoryRegion & operator=(const MemoryRegion &) = delete;
    MemoryRegion(const MemoryRegion &) = delete;
    MemoryRegion(MemoryRegion &&) = delete;

    virtual bool isAddressed(uint16_t address) const;
    virtual inline bool isWritable() const { return true; }

    virtual void write(uint16_t address, uint8_t value);
    virtual uint8_t & read(uint16_t address);

    virtual inline uint16_t size() const { return m_size; }

    virtual void reset() { std::fill(m_memory.begin(), m_memory.end(), resetValue()); }

    inline void enableInit()  { m_initializing = true;  }
    inline void disableInit() { m_initializing = false; }

    virtual inline uint8_t resetValue() const { return 0x00; }

protected:
    uint16_t m_offset;
    uint16_t m_size;

    bool m_initializing;

    std::vector<uint8_t> m_memory;
};

#endif
