#include <cassert>

#include "memoryregion.h"
#include "memorycontroller.h"

using std::vector;

MemoryRegion::MemoryRegion(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset,
    uint8_t banks,
    uint16_t address)
    : m_parent(parent),
      m_offset(offset),
      m_size(size),
      m_banks(banks + 1),
      m_address(address),
      m_initializing(false),
      m_memory(size)
{
    for (auto & bank : m_memory) { bank.resize(size); }
}

MemoryRegion::MemoryRegion(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(parent, size, offset, 0, 0x0000)
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

vector<uint8_t> & MemoryRegion::getSelectedBank(uint16_t index)
{
    if (!isBankingActive(index)) { return m_memory[index / m_memory.size()]; }

    uint8_t selected = m_parent.read(m_address);
    assert(selected < m_memory.size());

    return m_memory[std::min(selected, uint8_t(m_memory.size()))];
}

void MemoryRegion::write(uint16_t address, uint8_t value)
{
    uint16_t index = address - m_offset;

    auto & bank = getSelectedBank(index);
    bank[index % bank.size()] = value;
}

uint8_t & MemoryRegion::read(uint16_t address)
{
    uint16_t index = address - m_offset;

    auto & bank = getSelectedBank(index);
    return bank[index % bank.size()];
}
