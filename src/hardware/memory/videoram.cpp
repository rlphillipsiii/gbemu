#include "memorycontroller.h"
#include "videoram.h"

const uint16_t VideoRam::BANK_SELECT_ADDRESS = 0xFF4F;

VideoRam::VideoRam(MemoryController & parent, uint16_t size, uint16_t offset)
    : MemoryRegion(size, offset),
      m_parent(parent),
      m_bank(size)
{

}

uint8_t & VideoRam::read(uint16_t address)
{
    if (!m_parent.isCGB()) { return MemoryRegion::read(address); }

    auto & bank = (m_parent.read(BANK_SELECT_ADDRESS)) ? m_memory : m_bank;
    return bank[address];
}

void VideoRam::write(uint16_t address, uint8_t value)
{
    if (m_parent.isCGB()) {
        auto & bank = (m_parent.read(BANK_SELECT_ADDRESS)) ? m_memory : m_bank;
        bank[address] = value;
    } else {
        MemoryRegion::write(address, value);
    }
}
