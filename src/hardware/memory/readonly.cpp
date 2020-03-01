#include "readonly.h"
#include "logging.h"
#include "memorycontroller.h"

ReadOnly::ReadOnly(MemoryController & parent, uint16_t size, uint16_t offset)
    : MemoryRegion(parent, size, offset)
{

}

void ReadOnly::write(uint16_t address, uint8_t value)
{
    if (!m_initializing) {
        LOG("Illegal write to address 0x%04x => 0x%02x\n", address, value);
        return;
    }
    MemoryRegion::write(address, value);
}
