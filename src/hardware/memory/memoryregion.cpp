#include "memoryregion.h"

MemoryRegion::MemoryRegion(uint16_t size, uint16_t offset)
    : m_offset(offset),
      m_size(size),
      m_initializing(false),
      m_memory(size)
{

}

bool MemoryRegion::isAddressed(uint16_t address) const
{
    return ((address >= m_offset) && (address < (m_offset + m_size)));
}

void MemoryRegion::write(uint16_t address, uint8_t value)
{
    m_memory[address - m_offset] = value;
}

uint8_t & MemoryRegion::read(uint16_t address)
{
    return m_memory[address - m_offset];
}
