#include "mappedio.h"
#include "memorycontroller.h"
#include "memmap.h"

MappedIO::MappedIO(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(parent, size, offset),
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

    case GPU_BG_PALETTE_DATA: {
        uint8_t pointer = m_parent.read(GPU_BG_PALETTE_INDEX);

        // TODO: need to write the data to the background palette area
        printf("GPU BG Data: %d ==> %d\n", pointer, value);

        // If the MSB of the bg palette index is set, then we need to auto increment
        // the pointer that we just read out of the register, which is contained in
        // the lower 6 bits of the register and then write it back to the register.
        if (pointer & 0x80) {
            m_parent.write(GPU_BG_PALETTE_INDEX, (0x80 + ((pointer + 1) & 0x3F)));
        }
        break;
    }

    case GPU_SPRITE_PALETTE_DATA: {
        uint8_t pointer = m_parent.read(GPU_SPRITE_PALETTE_INDEX);

        // TODO: need to write the data to the sprite palette area
        printf("GPU Sprite Data: %d ==> %d\n", pointer, value);

        // If the MSB of the sprite palette index is set, then we need to auto
        // increment the pointer that we just read out of the register, which is
        // contained in the lower 6 bits of the register and then write it back
        // to the register.
        if (pointer & 0x80) {
            m_parent.write(GPU_SPRITE_PALETTE_INDEX, (0x80 + ((pointer + 1) & 0x3F)));
        }
        break;
    }

    case GPU_DMA_OAM: {
        uint16_t source = uint16_t(value) * 0x0100;
        for (uint16_t i = 0; i < GRAPHICS_RAM_SIZE; i++) {
            m_parent.write(GRAPHICS_RAM_OFFSET + i, m_parent.read(source + i));
        }

        break;
    }

    case GPU_DMA_MODE: {
        uint16_t sLower = m_parent.read(GPU_DMA_SRC_LOW);
        uint16_t sUpper = m_parent.read(GPU_DMA_SRC_HIGH);
        uint16_t dLower = m_parent.read(GPU_DMA_DEST_LOW);
        uint16_t dUpper = m_parent.read(GPU_DMA_DEST_HIGH);

        uint16_t source = ((sUpper << 8) | sLower) & 0xFFF0;
        uint16_t dest   = ((dUpper << 8) | dLower) & 0xFFF0;

        uint8_t mode = m_parent.read(GPU_DMA_MODE) & 0x80;
        if (!mode) {
            uint16_t length = (value + 1) * 0x10;
            for (uint16_t i = 0; i < length; i++) {
                m_parent.write(dest + i, m_parent.read(source + i));
            }
            MemoryRegion::write(GPU_DMA_MODE, 0xFF);
        } else {
            // TODO: h-blank DMA

            MemoryRegion::write(address, (mode & (value & 0x7F)));
        }

        break;
    };

    case SERIAL_TX_CONTROL_ADDRESS: {
        break;
    }

    default:
        MemoryRegion::write(address, value);
        break;
    }
}
