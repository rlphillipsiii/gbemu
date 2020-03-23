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
      m_initializing(false),
      m_memory(banks + 1)
{
    resize(size);
}

MemoryRegion::MemoryRegion(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(parent, size, offset, 0)
{
}

size_t MemoryRegion::length() const
{
    if (m_memory.empty()) { return 0; }

    return (m_memory.size() * m_memory.at(0).size());
}

uint8_t & MemoryRegion::operator[](uint32_t index)
{
    assert(!m_memory.empty());

    size_t bank = index / m_memory.at(0).size();
    size_t idx  = index % m_memory.at(0).size();
    return m_memory.at(bank).at(idx);
}

bool MemoryRegion::isAddressed(uint16_t address) const
{
    return ((address >= m_offset) && (address < (m_offset + m_size)));
}

void MemoryRegion::resize(uint16_t size)
{
    for (auto & bank : m_memory) {
        bank.resize(size);
    }
    m_size = size;
}

void MemoryRegion::reset()
{
    for (auto & bank : m_memory) {
        std::fill(bank.begin(), bank.end(), resetValue());
    }
}

void MemoryRegion::write(uint16_t address, uint8_t value)
{
    assert(!m_memory.empty());
    assert(size_t(address - m_offset) < m_memory[0].size());

    m_memory[0][address - m_offset] = value;
}

uint8_t & MemoryRegion::read(uint16_t address)
{
    assert(!m_memory.empty());
    assert(size_t(address - m_offset) < m_memory[0].size());

    return m_memory[0][address - m_offset];
}
