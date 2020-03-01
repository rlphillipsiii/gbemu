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
#include <optional>
#include <functional>

#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"

using std::vector;
using std::string;
using std::ofstream;
using std::optional;
using std::reference_wrapper;

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

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

MemoryController::MemoryController()
    : m_bios(BIOS_SIZE, BIOS_OFFSET),
      m_vram(*this, GPU_RAM_SIZE, GPU_RAM_OFFSET),
      m_working(WORKING_RAM_SIZE, WORKING_RAM_OFFSET),
      m_oam(GRAPHICS_RAM_SIZE, GRAPHICS_RAM_OFFSET),
      m_io(*this, IO_SIZE, IO_OFFSET),
      m_zero(ZRAM_SIZE, ZRAM_OFFSET),
      m_unusable(UNUSABLE_MEM_SIZE, UNUSABLE_MEM_OFFSET),
      m_cgb(false)
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
        m_bios, m_cartridge, m_vram, m_working, m_oam, m_unusable, m_io, m_zero
    };

    for (auto it = std::next(m_memory.begin()); it != m_memory.end(); ++it) {
        (*it).get().reset();
    }
}

void MemoryController::setCartridge(const string & filename)
{
    m_cartridge.load(filename);
}

optional<reference_wrapper<MemoryRegion>> MemoryController::find(uint16_t address) const
{
    for (auto & region : m_memory) {
        if (region.get().isAddressed(address)) {
            return region;
        }
    }

    return std::nullopt;
}

void MemoryController::initialize(uint16_t address, uint8_t value)
{
    auto found = find(address);
    if (!found) { return; }

    auto & region = found->get();
    region.enableInit();
    region.write(address, value);
    region.disableInit();
}

void MemoryController::write(uint16_t address, uint8_t value)
{
    auto found = find(address);
    if (!found) {
        LOG("MemoryController::write - Unhandled address 0x%04x\n", address);
        assert(0);
        return;
    }

    auto & region = found->get();
    if (region.isWritable()) {
        region.write(address, value);
    }
}

uint8_t & MemoryController::read(uint16_t address)
{
    auto region = find(address);
    if (region) {
        return region->get().read(address);
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
