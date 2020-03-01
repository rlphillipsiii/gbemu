#include "workingram.h"

WorkingRam::WorkingRam(uint16_t size, uint16_t offset)
    : MemoryRegion(size, offset),
      m_shadowOffset(offset + size),
      m_shadow(size - 512)
{

}

void WorkingRam::write(uint16_t address, uint8_t value)
{
    if (isShadowAddressed(address)) {
        m_shadow[address - m_shadowOffset] = value;
    } else {
        MemoryRegion::write(address, value);
    }
}

uint8_t & WorkingRam::read(uint16_t address)
{
    if (isShadowAddressed(address)) {
        return m_shadow[address - m_shadowOffset];
    }
    return MemoryRegion::read(address);
}

bool WorkingRam::isAddressed(uint16_t address) const
{
    bool addressed = MemoryRegion::isAddressed(address);
    if (!addressed) {
        addressed = isShadowAddressed(address);
    }
    return addressed;
}

bool WorkingRam::isShadowAddressed(uint16_t address) const
{
    return ((address >= m_shadowOffset) && (address < (m_shadowOffset + m_shadow.size())));
}
