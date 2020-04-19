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
#include <algorithm>

#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"
#include "gpu.h"
#include "dmgbios.h"
#include "cgbbios.h"
#include "memaccess.h"

using std::vector;
using std::string;
using std::ofstream;
using std::optional;
using std::reference_wrapper;
using std::ifstream;
using std::unique_ptr;

uint8_t MemoryController::DUMMY = 0xFF;

const uint16_t MemoryController::MBC_TYPE_ADDRESS = 0x0147;

MemoryController::MemoryController(
    GPU & gpu,
    MemAccessListener & listener,
    unique_ptr<ConsoleLink> & link)
    : m_bios(*this, 0, BIOS_OFFSET),
      m_cartridge(*this),
      m_working(*this, WORKING_RAM_SIZE, WORKING_RAM_OFFSET),
      m_oam(*this, GRAPHICS_RAM_SIZE, GRAPHICS_RAM_OFFSET),
      m_io(*this, gpu, link, IO_SIZE, IO_OFFSET),
      m_zero(*this, ZRAM_SIZE, ZRAM_OFFSET),
      m_unusable(*this, UNUSABLE_MEM_SIZE, UNUSABLE_MEM_OFFSET),
      m_gpu(gpu),
      m_listener(listener)
{
    init();
}

void MemoryController::init()
{
    m_memory = {
        m_bios, m_cartridge, m_gpu, m_working, m_oam, m_unusable, m_io, m_zero
    };
}

void MemoryController::reset()
{
    init();

    for (auto & region : m_memory) {
        region.get().reset();
    }
}

void MemoryController::setCartridge(const string & filename)
{
    reset();

    m_cartridge.load(filename);

    // We just changed the cartridge, so we are going to change the BIOS to
    // match whether or not the cartridge we just loaded is a CGB game or
    // not
    const auto & image = (m_cartridge.isCGB()) ? CGB_BIOS_REGION : DMG_BIOS_REGION;

    // We are going to rewrite the BIOS memory, which has no banking, so we
    // always need to grab the first entry in the memory vector.
    m_bios.resize(uint16_t(image.size()));
    std::copy(image.begin(), image.end(), m_bios.memory()[0].begin());
}

optional<reference_wrapper<MemoryRegion>> MemoryController::find(uint16_t address) const
{
    for (auto & region : m_memory) {
        if (region.get().isAddressed(address)) {
            return region;
        }
    }

    ERROR("Failed to find memory region for address 0x%04x\n", address);
    return std::nullopt;
}

void MemoryController::logAccess(GB::MemAccessType type, uint16_t address)
{
    if (!inBios()) {
        m_listener.onMemoryAccess(type, address);
    }
}

void MemoryController::initialize(uint16_t address, uint8_t value)
{
    auto found = find(address);
    if (!found) { return; }

    logAccess(GB::MEM_ACCESS_WRITE, address);

    auto & region = found->get();
    region.enableInit();
    region.write(address, value);
    region.disableInit();
}

void MemoryController::write(uint16_t address, uint8_t value)
{
    auto found = find(address);
    if (!found) {
        FATAL("Unhandled address 0x%04x\n", address);
        return;
    }

    logAccess(GB::MEM_ACCESS_WRITE, address);

    auto & region = found->get();
    if (region.isWritable()) {
        region.write(address, value);
    } else {
        WARN("Attempted to write to non-writable region: 0x%04x\n", address);
    }
}

const uint8_t & MemoryController::peek(uint16_t address)
{
    logAccess(GB::MEM_ACCESS_READ, address);

    auto region = find(address);
    if (region) {
        return region->get().read(address);
    }

    FATAL("Unhandled address 0x%04x\n", address);
    return DUMMY;
}

uint8_t & MemoryController::read(uint16_t address)
{
    logAccess(GB::MEM_ACCESS_READ, address);

    auto found = find(address);
    if (found) {
        auto & region = found->get();
        if (!region.isReadOnly()) {
            return region.read(address);
        }
    }

    FATAL("Unhandled address 0x%04x\n", address);
    return DUMMY;
}

void MemoryController::saveBIOS(const string & filename)
{
    ofstream out;
    out.open(filename, std::ios::out | std::ios::binary);

    auto & bios = m_bios.memory()[0];

    out.write(reinterpret_cast<const char*>(bios.data()), bios.size());
    out.close();
}
