#include <cassert>

#include "memoryregion.h"
#include "memorycontroller.h"

using std::vector;

MemoryRegion::MemoryRegion(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset,
    uint8_t banks)
    : m_parent(parent),
      m_offset(offset),
      m_size(size),
      m_banks(banks + 1),
      m_initializing(false),
      m_memory(banks + 1)
{
    for (auto & bank : m_memory) { bank.resize(size); }
}

MemoryRegion::MemoryRegion(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(parent, size, offset, 0)
{
}

bool MemoryRegion::isAddressed(uint16_t address) const
{
    return ((address >= m_offset) && (address < (m_offset + m_size)));
}

void MemoryRegion::reset()
{
    for (auto & bank : m_memory) {
        std::fill(bank.begin(), bank.end(), resetValue());
    }
}

void MemoryRegion::write(uint16_t address, uint8_t value)
{
    m_memory[0][address - m_offset] = value;
}

uint8_t & MemoryRegion::read(uint16_t address)
{
    return m_memory[0][address - m_offset];
}
