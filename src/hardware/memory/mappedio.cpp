#include "mappedio.h"
#include "memorycontroller.h"
#include "memmap.h"

MappedIO::MappedIO(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(size, offset),
      m_parent(parent),
      m_rtcReset(false)
{
    MemoryRegion::write(JOYPAD_INPUT_ADDRESS, 0xFF);
}

void MappedIO::write(uint16_t address, uint8_t value)
{
    if (m_initializing) { MemoryRegion::write(address, value); return; }

    switch (address) {
    case GPU_STATUS_ADDRESS:        writeBytes(address, value, 0x78); break;
    case JOYPAD_INPUT_ADDRESS:      writeBytes(address, value, 0x30); break;
    case INTERRUPT_MASK_ADDRESS:    writeBytes(address, value, 0x1F); break;
    case INTERRUPT_FLAGS_ADDRESS:   writeBytes(address, value, 0x1F); break;
    case CPU_TIMER_CONTROL_ADDRESS: writeBytes(address, value, 0x07); break;

    case CPU_TIMER_DIV_ADDRESS: {
        m_rtcReset = true;
        break;
    }

    case GPU_OAM_DMA: {
        uint16_t source = uint16_t(value) * 0x0100;
        for (uint16_t i = 0; i < GRAPHICS_RAM_SIZE; i++) {
            m_parent.write(GRAPHICS_RAM_OFFSET + i, m_parent.read(source + i));
        }

        break;
    }

    case SERIAL_TX_CONTROL_ADDRESS: {
        break;
    }

    default:
        MemoryRegion::write(address, value);
        break;
    }
}
