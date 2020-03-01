#ifndef _MEMORY_REGION_H
#define _MEMORY_REGION_H

#include <vector>
#include <cstdint>

class MemoryController;

class MemoryRegion {
public:
    explicit MemoryRegion(
        MemoryController & parent,
        uint16_t size,
        uint16_t offset);
    explicit MemoryRegion(
        MemoryController & parent,
        uint16_t size,
        uint16_t offset,
        uint8_t banks,
        uint16_t address);
    virtual ~MemoryRegion() = default;

    MemoryRegion & operator=(const MemoryRegion &) = delete;
    MemoryRegion(const MemoryRegion &) = delete;
    MemoryRegion(MemoryRegion &&) = delete;
    MemoryRegion(const MemoryRegion &&) = delete;

    virtual bool isAddressed(uint16_t address) const;
    virtual inline bool isWritable() const { return true; }
    virtual inline bool isBankingActive(uint16_t) const { return false; }

    virtual void write(uint16_t address, uint8_t value);
    virtual uint8_t & read(uint16_t address);

    virtual inline uint16_t size() const { return m_size; }

    virtual void reset();

    inline void enableInit()  { m_initializing = true;  }
    inline void disableInit() { m_initializing = false; }

    virtual inline uint8_t resetValue() const { return 0x00; }

protected:
    MemoryController & m_parent;

    uint16_t m_offset;
    uint16_t m_size;
    uint8_t m_banks;

    uint16_t m_address;

    bool m_initializing;

    std::vector<std::vector<uint8_t>> m_memory;

    std::vector<uint8_t> & getSelectedBank(uint16_t index);
};

#endif
