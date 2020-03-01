#include "memorycontroller.h"
#include "videoram.h"

const uint8_t VideoRam::BANK_COUNT = 1;

const uint16_t VideoRam::BANK_SELECT_ADDRESS = 0xFF4F;

VideoRam::VideoRam(MemoryController & parent, uint16_t size, uint16_t offset)
    : MemoryRegion(parent, size, offset, BANK_COUNT, BANK_SELECT_ADDRESS)
{

}

bool VideoRam::isBankingActive(uint16_t) const
{
    return m_parent.isCGB();
}
