#include "workingram.h"
#include "memorycontroller.h"

const uint8_t WorkingRam::BANK_COUNT = 7;

const uint16_t WorkingRam::BANK_SELECT_ADDRESS = 0xFF70;

WorkingRam::WorkingRam(MemoryController & parent, uint16_t size, uint16_t offset)
    : MemoryRegion(parent, size / 2, offset, BANK_COUNT, BANK_SELECT_ADDRESS)
{

}

bool WorkingRam::isBankingActive(uint16_t index) const
{
    if (!m_parent.isCGB()) { return false; }

    return (index >= m_size);
}

void WorkingRam::write(uint16_t address, uint8_t value)
{
    if (isShadowAddressed(address)) {
        address -= (m_size * 2);
    }
    MemoryRegion::write(address, value);
}

uint8_t & WorkingRam::read(uint16_t address)
{
    return MemoryRegion::read(address);
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
    const uint16_t lower = m_offset + m_size;
    const uint16_t upper = lower + m_size - 512;

    return ((address >= lower) && (address < upper));
}
