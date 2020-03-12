#include <cassert>

#include "workingram.h"
#include "memorycontroller.h"

using std::vector;

const uint8_t WorkingRam::BANK_COUNT = 8;

const uint16_t WorkingRam::BANK_SELECT_ADDRESS = 0xFF70;

WorkingRam::WorkingRam(MemoryController & parent, uint16_t size, uint16_t offset)
    : MemoryRegion(parent, size / 2, offset, BANK_COUNT - 1)
{
}

vector<uint8_t> & WorkingRam::bank(uint16_t index)
{
    if (!m_parent.isCGB()) { return m_memory[index / m_size]; }

    uint8_t selected =
        (index < m_size) ? 0 : std::max(0x01, int(m_parent.peek(BANK_SELECT_ADDRESS)));
    assert(size_t(selected) < m_memory.size());

    return m_memory[selected];
}

void WorkingRam::write(uint16_t address, uint8_t value)
{
    if (isShadowAddressed(address)) {
        address -= (m_size * 2);
    }

    uint16_t index = address - m_offset;
    assert(index < (m_size * 2));

    bank(index)[index % m_size] = value;
}

uint8_t & WorkingRam::read(uint16_t address)
{
    if (isShadowAddressed(address)) {
        address -= (m_size * 2);
    }

    uint16_t index = address - m_offset;
    assert(index < (m_size * 2));

    return bank(index)[index % m_size];
}

bool WorkingRam::isAddressed(uint16_t address) const
{
    const uint16_t lower = m_offset;
    const uint16_t upper = lower + (m_size * 2);

    bool addressed = ((address >= lower) && (address < upper));
    if (!addressed) {
        addressed = isShadowAddressed(address);
    }
    return addressed;
}

bool WorkingRam::isShadowAddressed(uint16_t address) const
{
    const uint16_t lower = m_offset + (m_size * 2);
    const uint16_t upper = lower + ((m_size * 2) - 512);

    return ((address >= lower) && (address < upper));
}
