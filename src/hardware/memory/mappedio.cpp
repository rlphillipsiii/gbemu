#include <memory>

#include "mappedio.h"
#include "memorycontroller.h"
#include "memmap.h"
#include "gpu.h"
#include "consolelink.h"

using std::unique_ptr;

MappedIO::MappedIO(
    MemoryController & memory,
    GPU & gpu,
    unique_ptr<ConsoleLink> & link,
    uint16_t size,
    uint16_t offset)
    : MemoryRegion(memory, size, offset),
      m_gpu(gpu),
      m_link(link),
      m_rtcReset(false)
{
    MemoryRegion::write(JOYPAD_INPUT_ADDRESS, 0xFF);
}

uint8_t & MappedIO::ref(uint16_t address)
{
    switch (address) {
    case GPU_BG_PALETTE_DATA: {
        uint8_t pointer = m_parent.read(GPU_BG_PALETTE_INDEX) & 0x3F;

        return m_gpu.readBgPalette(pointer);
    }

    case GPU_SPRITE_PALETTE_DATA: {
        uint8_t pointer = m_parent.read(GPU_SPRITE_PALETTE_INDEX) & 0x3F;

        return m_gpu.readSpritePalette(pointer);
    }
    default:break;
    }

    return MemoryRegion::ref(address);
}

uint8_t MappedIO::read(uint16_t address)
{
    switch (address) {
    case GPU_STATUS_ADDRESS: {
        uint8_t data = ref(address);
        if (!m_gpu.display()) {
            data &= 0xFC;
        }
        return data;
    }

    case GPU_SCANLINE_ADDRESS: {
        return (m_gpu.display()) ? ref(address) : 0x00;
    }

    default: break;
    }

    return ref(address);
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

        m_gpu.writeBgPalette(pointer & 0x3F, value);

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

        m_gpu.writeSpritePalette(pointer & 0x3F, value);

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
        m_gpu.dma(value);
        break;
    };

    case SERIAL_CONTROL_ADDRESS: {
        constexpr const uint8_t mask =
            (ConsoleLink::LINK_CLOCK | ConsoleLink::LINK_SPEED | ConsoleLink::LINK_TRANSFER);

        writeBytes(address, value, mask);

        if (value & ConsoleLink::LINK_TRANSFER) {
            uint8_t data = MemoryRegion::read(SERIAL_DATA_ADDRESS);
            if (m_link) {
                m_link->transfer(data);
            }
        }
        break;
    }

    default:
        MemoryRegion::write(address, value);
        break;
    }
}
