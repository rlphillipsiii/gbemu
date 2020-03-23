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
        uint8_t banks);
    virtual ~MemoryRegion() = default;

    MemoryRegion & operator=(const MemoryRegion &) = delete;
    MemoryRegion(const MemoryRegion &) = delete;
    MemoryRegion(MemoryRegion &&) = delete;
    MemoryRegion(const MemoryRegion &&) = delete;

    virtual bool isAddressed(uint16_t address) const;

    virtual inline bool isWritable() const { return true;  }
    virtual inline bool isReadOnly() const { return false; }

    virtual void write(uint16_t address, uint8_t value);
    virtual uint8_t & read(uint16_t address);

    virtual inline uint16_t size() const { return m_size; }
    inline uint16_t offset() const { return m_offset; }

    virtual void reset();

    inline void enableInit()  { m_initializing = true;  }
    inline void disableInit() { m_initializing = false; }

    virtual inline uint8_t resetValue() const { return 0x00; }

    void resize(uint16_t size);

    inline std::vector<std::vector<uint8_t>> & memory() { return m_memory; }

    uint8_t & operator[](uint32_t index);
    size_t length() const;

protected:
    MemoryController & m_parent;

    uint16_t m_offset;
    uint16_t m_size;

    bool m_initializing;

    std::vector<std::vector<uint8_t>> m_memory;
};

#endif
