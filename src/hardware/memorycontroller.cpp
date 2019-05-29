/*
 * memorycontroller.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <cstdio>
#include <cassert>

#include "memorycontroller.h"
#include "memmap.h"

uint8_t MemoryController::DUMMY = 0xFF;

////////////////////////////////////////////////////////////////////////////////
MemoryController::Region::Region(uint16_t size, uint16_t offset)
    : m_offset(offset),
      m_memory(size)
{

}

bool MemoryController::Region::isAddressed(uint16_t address) const
{
    return ((address >= m_offset) && (address < (m_offset + m_memory.size())));
}

void MemoryController::Region::write(uint16_t address, uint8_t value)
{
    m_memory[address - m_offset] = value;
}

uint8_t & MemoryController::Region::read(uint16_t address)
{
    return m_memory[address - m_offset];
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
MemoryController::WorkingRam::WorkingRam(uint16_t size, uint16_t offset)
    : Region(size, offset),
      m_shadowOffset(offset + size),
      m_shadow(size - 512)
{

}

void MemoryController::WorkingRam::write(uint16_t address, uint8_t value)
{
    if (isShadowAddressed(address)) {
        m_shadow[address - m_shadowOffset] = value;
    } else {
        Region::write(address, value);
    }
}

uint8_t & MemoryController::WorkingRam::read(uint16_t address)
{
    if (isShadowAddressed(address)) {
        return m_shadow[address - m_shadowOffset];
    }
    return Region::read(address);
}

bool MemoryController::WorkingRam::isAddressed(uint16_t address) const
{
    bool addressed = Region::isAddressed(address);
    if (!addressed) {
        addressed = isShadowAddressed(address);
    }
    return addressed;
}

bool MemoryController::WorkingRam::isShadowAddressed(uint16_t address) const
{
    return ((address >= m_shadowOffset) && (address < (m_shadowOffset + m_shadow.size())));
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void MemoryController::MemoryMappedIO::write(uint16_t address, uint8_t value)
{
    (void)address; (void)value;
}

uint8_t & MemoryController::MemoryMappedIO::read(uint16_t address)
{
    static uint8_t temp = 0x00;
    (void)address;
    return temp;
}
////////////////////////////////////////////////////////////////////////////////

MemoryController::MemoryController()
    : m_bios(BIOS_SIZE, BIOS_OFFSET),
      m_rom_0(ROM_0_SIZE, ROM_0_OFFSET),
      m_rom_1(ROM_1_SIZE, ROM_1_OFFSET),
      m_gpu(GPU_RAM_SIZE, GPU_RAM_OFFSET),
      m_ext(EXT_RAM_SIZE, EXT_RAM_OFFSET),
      m_working(WORKING_RAM_SIZE, WORKING_RAM_OFFSET),
      m_io(IO_SIZE, IO_OFFSET),
      m_zero(ZRAM_SIZE, ZRAM_OFFSET)
{
    reset();
}

void MemoryController::reset()
{
    m_memory = { &m_bios, &m_rom_0, &m_rom_1, &m_gpu, &m_ext, &m_working, &m_io, &m_zero };
}

void MemoryController::write(uint16_t address, uint8_t value)
{
    Region *region = nullptr;
    for (Region *r : m_memory) {
        if (r->isAddressed(address)) {
            region = r;
            break;
        }
    }

    if (region) {
        region->write(address, value);
    } else {
        printf("MemoryController::write - Unhandled address 0x%04x\n", address);
        assert(0);
    }
}

uint8_t & MemoryController::read(uint16_t address)
{
    Region *region = nullptr;
    for (Region *r : m_memory) {
        if (region->isAddressed(address)) {
            region = r;
            break;
        }
    }

    if (region) {
        return region->read(address);
    }

    printf("MemoryController::read - Unhandled address 0x%04x\n", address);
    assert(0);

    return DUMMY;
}

void MemoryController::writeWord(uint16_t address, uint16_t value)
{
    uint16_t low  = value & 0xFF;
    uint16_t high = (value >> 8) & 0xFF;

    write(address, low);
    write(address + 1, high);
}

uint16_t MemoryController::readWord(uint16_t address)
{
    uint16_t low  = read(address);
    uint16_t high = read(address + 1);

    return (high << 8) | low;
}

void MemoryController::unlockBiosRegion()
{
    m_memory.pop_front();
}

