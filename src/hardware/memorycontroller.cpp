/*
 * memorycontroller.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <cstdio>
#include <cassert>
#include <vector>
#include <string>
#include <fstream>

#include "memorycontroller.h"
#include "memmap.h"
#include "gpu.h"
#include "logging.h"
#include "cartridge.h"

using std::vector;
using std::string;
using std::ofstream;

uint8_t MemoryController::DUMMY = 0xFF;

const uint16_t MemoryController::MBC_TYPE_ADDRESS = 0x0147;

const vector<uint8_t> MemoryController::BIOS_REGION = {
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50,
};

////////////////////////////////////////////////////////////////////////////////
MemoryController::Region::Region(uint16_t size, uint16_t offset)
    : m_offset(offset),
      m_size(size),
      m_initializing(false),
      m_memory(size)
{

}

bool MemoryController::Region::isAddressed(uint16_t address) const
{
    return ((address >= m_offset) && (address < (m_offset + m_size)));
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
MemoryController::ReadOnly::ReadOnly(uint16_t size, uint16_t offset)
    : Region(size, offset)
{

}

void MemoryController::ReadOnly::write(uint16_t address, uint8_t value)
{
    if (!m_initializing) {
        LOG("Illegal write to address 0x%04x => 0x%02x\n", address, value);
        return;
    }
    Region::write(address, value);
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
uint8_t MemoryController::Removable::EMPTY = 0xFF;

void MemoryController::Removable::load(const string & filename)
{
    m_cartridge = std::make_shared<Cartridge>(filename);
}

void MemoryController::Removable::write(uint16_t address, uint8_t value)
{
    if (m_cartridge && m_cartridge->isValid()) {
        m_cartridge->write(address, value);
    }
}

uint8_t & MemoryController::Removable::read(uint16_t address)
{
    return (m_cartridge && m_cartridge->isValid())
        ? m_cartridge->read(address) : EMPTY;
}

bool MemoryController::Removable::isAddressed(uint16_t address) const
{
    if (address < (ROM_0_OFFSET + ROM_0_SIZE)) {
        return true;
    }
    if ((address >= ROM_1_OFFSET) && (address < (ROM_1_OFFSET + ROM_1_SIZE))) {
        return true;
    }
    if ((address >= EXT_RAM_OFFSET) && (address < (EXT_RAM_OFFSET + EXT_RAM_SIZE))) {
        return true;
    }
    
    return false;
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
MemoryController::MemoryMappedIO::MemoryMappedIO(
    MemoryController & parent,
    uint16_t size,
    uint16_t offset)
    : Region(size, offset),
      m_parent(parent),
      m_rtcReset(false)
{
    Region::write(JOYPAD_INPUT_ADDRESS, 0xFF);
}

void MemoryController::MemoryMappedIO::reset()
{

}

void MemoryController::MemoryMappedIO::write(uint16_t address, uint8_t value)
{
    if (m_initializing) { Region::write(address, value); return; }
    
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
        Region::write(address, value);
        break;
    }
}
////////////////////////////////////////////////////////////////////////////////

MemoryController::MemoryController()
    : m_bios(BIOS_SIZE, BIOS_OFFSET),
      m_gram(GPU_RAM_SIZE, GPU_RAM_OFFSET),
      m_working(WORKING_RAM_SIZE, WORKING_RAM_OFFSET),
      m_graphics(GRAPHICS_RAM_SIZE, GRAPHICS_RAM_OFFSET),
      m_io(*this, IO_SIZE, IO_OFFSET),
      m_zero(ZRAM_SIZE, ZRAM_OFFSET),
      m_unusable(UNUSABLE_MEM_SIZE, UNUSABLE_MEM_OFFSET)
{
    reset();

    assert(uint16_t(BIOS_REGION.size()) == m_bios.size());

    m_bios.enableInit();
    
    uint16_t length = std::min(uint16_t(BIOS_REGION.size()), m_bios.size());
    for (uint16_t i = 0; i < length; i++) {
        m_bios.write(i, BIOS_REGION[i]);
    }

    m_bios.disableInit();
}

void MemoryController::reset()
{
    m_memory = {
        &m_bios, &m_cartridge, &m_gram, &m_working, &m_graphics, &m_unusable, &m_io, &m_zero
    };

    for (Region *region : m_memory) {
        if (&m_bios == region) {
            continue;
        }
        region->reset();
    }
}

void MemoryController::setCartridge(const string & filename)
{
    m_cartridge.load(filename);
}

MemoryController::Region *MemoryController::find(uint16_t address) const
{
    for (Region *region : m_memory) {
        if (region->isAddressed(address)) {
            return region;
        }
    }

    return nullptr;
}

void MemoryController::initialize(uint16_t address, uint8_t value)
{
    Region *region = find(address);

    if (region) {
        region->enableInit();
        region->write(address, value);
        region->disableInit();
    }
}

void MemoryController::write(uint16_t address, uint8_t value)
{
    Region *region = find(address);

    if (region && region->isWritable()) {
        region->write(address, value);
        return;
    } 

    LOG("MemoryController::write - Unhandled address 0x%04x\n", address);
    assert(0);
}

uint8_t & MemoryController::read(uint16_t address)
{
    Region *region = find(address);
    if (region) {
        return region->read(address);
    }

    LOG("MemoryController::read - Unhandled address 0x%04x\n", address);
    assert(0);

    return DUMMY;
}

void MemoryController::saveBIOS(const string & filename)
{
    ofstream out;
    out.open(filename, std::ios::out | std::ios::binary);

    out.write(reinterpret_cast<const char*>(BIOS_REGION.data()), BIOS_REGION.size());
    out.close();
}
