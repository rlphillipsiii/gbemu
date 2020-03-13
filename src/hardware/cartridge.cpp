#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <memory>
#include <chrono>
#include <ctime>

#include "cartridge.h"
#include "logging.h"

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::unique_ptr;
using std::chrono::system_clock;
using std::time_t;

const uint16_t Cartridge::NINTENDO_LOGO_OFFSET = 0x0104;

const uint16_t Cartridge::ROM_HEADER_LENGTH   = 0x0180;
const uint16_t Cartridge::ROM_NAME_OFFSET     = 0x0134;
const uint16_t Cartridge::ROM_CGB_OFFSET      = 0x0143;
const uint16_t Cartridge::ROM_TYPE_OFFSET     = 0x0147;
const uint16_t Cartridge::ROM_SIZE_OFFSET     = 0x0148;
const uint16_t Cartridge::ROM_RAM_SIZE_OFFSET = 0x0149;

const uint8_t Cartridge::ROM_NAME_MAX_LENGTH = 0x10;

const vector<uint8_t> Cartridge::NINTENDO_LOGO = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03,
    0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
    0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
    0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
    0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB,
    0xB9, 0x33, 0x3E,
};

const vector<uint16_t> Cartridge::RAM_SIZES = {
    0x0000, 0x0800, 0x2000, 0x8000,
};

Cartridge::Cartridge(const string & path)
    : m_path(path),
      m_valid(false),
      m_cgb(false)
{
    ifstream input(m_path, std::ios::in | std::ios::binary);
    if (!input.is_open()) { return; }

    input.seekg(0, input.end);
    m_info.size = uint32_t(input.tellg());
    input.seekg(0, input.beg);

    // Sanity check.  This shouldn't ever really happen unless we are reading
    // something that isn't an actual ROM image.
    if (m_info.size < ROM_HEADER_LENGTH) { return; }

    m_memory.resize(m_info.size);
    input.read(reinterpret_cast<char*>(m_memory.data()), m_memory.size());

    // Check for the Nintendo logo.  If the Nintendo logo isn't in the correct
    // location, then the cartridge image is no good, and there is no point in
    // continuing to parse the rest of the header.
    for (size_t i = 0; i < NINTENDO_LOGO.size(); i++) {
        if (m_memory.at(i + NINTENDO_LOGO_OFFSET) != NINTENDO_LOGO.at(i)) {
            return;
        }
    }

    for (uint8_t i = 0; i < ROM_NAME_MAX_LENGTH; i++) {
        uint8_t entry = m_memory.at(ROM_NAME_OFFSET + i);
        if ((0x80 == entry) || (0xC0 == entry)) {
            break;
        }

        if (0x00 == entry) { continue; }

        m_info.name += char(entry);
    }

    // Read the memory bank type and use that to construct an object that
    // will handling reading/writing for this ROM.
    m_info.type = BankType(m_memory.at(ROM_TYPE_OFFSET));
    m_bank = unique_ptr<MemoryBankController>(initMemoryBankController(m_info.type));

    assert(m_bank);

    m_cgb = (m_memory.at(ROM_CGB_OFFSET) == 0xC0) || (m_memory.at(ROM_CGB_OFFSET) == 0x80);

    NOTE("ROM Name: %s\n", m_info.name.c_str());

    LOG("ROM Size:  0x%x (0x%02x)\n", m_info.size, m_memory.at(ROM_SIZE_OFFSET));
    LOG("ROM RAM Size: %dKB\n", int(m_bank->size() / 1024));
    LOG("Bank Type: %s (0x%02x)\n", m_bank->name().c_str(), uint8_t(m_info.type));
    m_valid = true;
}

Cartridge::MemoryBankController *Cartridge::initMemoryBankController(uint8_t type)
{
    MemoryBankController *bank = nullptr;

    uint8_t index = m_memory.at(ROM_RAM_SIZE_OFFSET);

    uint16_t size = (0 == index) ? *RAM_SIZES.rbegin() : RAM_SIZES.at(index);

    switch (type) {
    default:
        ERROR("Unknown memory bank type: 0x%02x\n", type);
        [[fallthrough]];

    case MBC_1:
    case MBC_1R:
        bank = new MBC1(*this, size, false);
        break;
    case MBC_1RB:
        bank = new MBC1(*this, size, true);
        break;

    case MBC_3RB:
    case MBC_3TRB:
        bank = new MBC3(*this, size, true);
        break;
    }

    return bank;
}

uint8_t & Cartridge::read(uint16_t address)
{
    assert(m_bank);

    // The lower half of the ROM address space always points to the beginning of
    // the ROM, so there is no need to have the MBC handle the read.  Just read it
    // out of the memory buffer.
    if (address < (ROM_0_OFFSET + ROM_0_SIZE)) {
        assert(address < m_memory.size());

        return m_memory[address];
    }
    if (address < (ROM_1_OFFSET + ROM_1_SIZE)) {
        return m_bank->readROM(address);
    }
    if ((address >= EXT_RAM_OFFSET) && (address < (EXT_RAM_OFFSET + EXT_RAM_SIZE))) {
        return m_bank->readRAM(address);
    }

    FATAL("Invalid cartridge read address: 0x%04x\n", address);

    static uint8_t EMPTY = 0xFF;
    return EMPTY;
}

void Cartridge::write(uint16_t address, uint8_t value)
{
    assert(m_bank);

    if ((address < (ROM_0_OFFSET + ROM_0_SIZE)) || (address < (ROM_1_OFFSET + ROM_1_SIZE))) {
        m_bank->writeROM(address, value);
    } else if ((address >= EXT_RAM_OFFSET) && (address < (EXT_RAM_OFFSET + EXT_RAM_SIZE))) {
        m_bank->writeRAM(address, value);
    }
}

////////////////////////////////////////////////////////////////////////////////
uint8_t Cartridge::MemoryBankController::RAM_DISABLED = 0xFF;

Cartridge::MemoryBankController::MemoryBankController(
    Cartridge & cartridge,
    const std::string & name,
    uint16_t size,
    bool battery)
    : m_cartridge(cartridge),
      m_name(name),
      m_ram(size / EXT_RAM_SIZE),
      m_ramEnable(false),
      m_romBank(1),
      m_ramBank(0),
      m_hasBattery(battery)
{
    initRAM(m_cartridge.game() + ".sav");
}

Cartridge::MemoryBankController::~MemoryBankController()
{
    if (m_nvRam.is_open()) { m_nvRam.close(); }
}

void Cartridge::MemoryBankController::initRAM(const string & name)
{
    if (!m_hasBattery) { return; }

    LOG("NV RAM Filename: %s\n", name.c_str());

    ifstream input(name, std::ios::in | std::ios::binary);
    if (input.is_open()) {
        for (auto & bank : m_ram) {
            input.read(reinterpret_cast<char*>(bank.data()), bank.size());
        }
        input.close();
    }

    m_nvRam = ofstream(name, std::ios::out | std::ios::binary);
    if (!m_nvRam) { return; }

    for (auto & bank : m_ram) {
        m_nvRam.write(reinterpret_cast<char*>(bank.data()), bank.size());
    }
    m_nvRam.flush();
}

void Cartridge::MemoryBankController::writeRAM(uint16_t address, uint8_t value)
{
    if (!m_ramEnable) { return; }

    uint16_t index = address - EXT_RAM_OFFSET;
    assert(index < m_ram[m_ramBank].size());

    m_ram[m_ramBank][index] = value;

    if (m_nvRam.good()) {
        m_nvRam.seekp(index + (EXT_RAM_SIZE * m_ramBank));
        if (m_nvRam.rdstate()) {
            ERROR("RAM seek failed: 0x%02x\n", m_nvRam.rdstate());
            return;
        }

        m_nvRam.write(reinterpret_cast<const char*>(&value), 1);
        m_nvRam.flush();
    }
}

uint8_t & Cartridge::MemoryBankController::readROM(uint16_t address)
{
    assert(m_romBank > 0);

    uint32_t index = address + ((m_romBank - 1) * ROM_1_SIZE);
    assert(index < m_cartridge.m_memory.size());

    return m_cartridge.m_memory[index];
}

uint8_t & Cartridge::MemoryBankController::readRAM(uint16_t address)
{
    if (!m_ramEnable) { return RAM_DISABLED; }

    uint16_t index = address - EXT_RAM_OFFSET;
    assert(index < m_ram[m_ramBank].size());

    return m_ram[m_ramBank][index];
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Cartridge::MBC1::MBC1(Cartridge & cartridge, uint16_t size, bool battery)
    : MemoryBankController(cartridge, "MBC1", size, battery),
      m_mode(Cartridge::MBC_ROM)
{

}

void Cartridge::MBC1::writeROM(uint16_t address, uint8_t value)
{
    if (address < 0x2000) {
        m_ramEnable = ((value & 0x0F) == 0x0A);
    } else if (address < 0x4000) {
        uint8_t bank = (value & 0x1F);

        // 0x00, 0x20, 0x40, and 0x60 are all illegal values that cause the bank to
        // get set to the next bank.
        if ((0x00 == bank) || (0x20 == bank) || (0x40 == bank) || (0x60 == bank)) {
            bank |= 0x01;
        }

        // Writes to this address set the lower 5 bits of the bank number, so or
        // this value with the upper 3 bits that are already there.
        m_romBank = ((m_romBank & 0xE0) | bank);
    } else if (address < 0x6000) {
        if (MBC_ROM == m_mode) {
            // If we are in ROM mode, then writes to this address set upper 3 bits
            // of the ROM bank number, so we need to or the lower 5 bits of the
            // current bank number with the value that we are writing shifted to
            // the left by 5.
            m_romBank = (((value << 5) & 0xE0) | m_romBank);
        } else {
            m_ramBank = (value & 0x07);
        }
    } else if (address < 0x8000) {
        m_mode = (0x00 == value) ? Cartridge::MBC_ROM : Cartridge::MBC_RAM;
    } else {
        LOG("Illegal MBC1 write to address 0x%04x\n", address);

        assert(0);
    }
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Cartridge::MBC3::MBC3(Cartridge & cartridge, uint16_t size, bool battery)
    : MemoryBankController(cartridge, "MBC3", size, battery),
      m_latch(0x00)
{

}

uint8_t & Cartridge::MBC3::readRAM(uint16_t address)
{
    if (m_ramBank <= 0x03) { return MemoryBankController::readRAM(address); }

    switch (m_ramBank) {
    default:
        ERROR("Unknown RAM bank setting: 0x%02x\n", m_ramBank);
        [[fallthrough]];

    case RtcSeconds:  return m_rtc.seconds;
    case RtcMinutes:  return m_rtc.minutes;
    case RtcHours:    return m_rtc.hours;
    case RtcDayLower: return m_rtc.day[0];
    case RtcDayUpper: return m_rtc.day[1];
    }
}

void Cartridge::MBC3::writeROM(uint16_t address, uint8_t value)
{
    if (address < 0x2000) {
        m_ramEnable = ((value & 0x0F) == 0x0A);
    } else if (address < 0x4000) {
        uint8_t bank = (value & 0x7F);
        m_romBank = (0x00 != bank) ? bank : 0x01;
    } else if (address < 0x6000) {
        m_ramBank = value;
    } else if (address < 0x8000) {
        uint8_t last = m_latch;
        m_latch = value;

        // We only want to latch the RTC values when a value of 0x01 is written
        // while the current value is 0x00, so if that isn't the case, then we
        // can just bail out here.
        if (!((0x00 == last) && (0x01 == m_latch))) { return; }

        time_t timestamp = system_clock::to_time_t(system_clock::now());

        std::tm *now = std::localtime(&timestamp);
        if (now) {
            m_rtc.seconds = now->tm_sec;
            m_rtc.minutes = now->tm_min;
            m_rtc.hours   = now->tm_hour;

            // The day is stored in 9 bits.  The lower 8 bits are stored in the
            // lower day register, and the MSB is stored in the LSB of the
            // upper day register.
            uint16_t days = now->tm_yday;
            m_rtc.day[0] = days & 0xFF;
            m_rtc.day[1] = (m_rtc.day[1] & 0xFE) | ((days >> 8) & 0x01);
        }
    } else {
        LOG("Illegal MBC3 write to address 0x%04x\n", address);

        assert(0);
    }
}
////////////////////////////////////////////////////////////////////////////////
