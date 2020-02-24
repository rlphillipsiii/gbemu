#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <memory>

#include "cartridge.h"
#include "logging.h"

using std::vector;
using std::string;
using std::ifstream;
using std::unique_ptr;

const uint16_t Cartridge::NINTENDO_LOGO_OFFSET = 0x0104;

const uint16_t Cartridge::ROM_HEADER_LENGTH   = 0x0180;
const uint16_t Cartridge::ROM_NAME_OFFSET     = 0x0134;
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

Cartridge::Cartridge(const string & path)
    : m_path(path),
      m_valid(false),
      m_ram(EXT_RAM_SIZE)
{
    ifstream input(m_path, std::ios::in | std::ios::binary);
    if (!input.is_open()) { return; }

    input.seekg(0, input.end);
    m_info.size = uint32_t(input.tellg());
    input.seekg(0, input.beg);

    if (m_info.size < ROM_HEADER_LENGTH) { return; }

    m_memory.resize(m_info.size);
    input.read(reinterpret_cast<char*>(m_memory.data()), m_memory.size());

    for (uint8_t i = 0; i < ROM_NAME_MAX_LENGTH; i++) {
        uint8_t entry = m_memory.at(ROM_NAME_OFFSET + i);
        if ((0x80 == entry) || (0xC0 == entry)) {
            break;
        }

        m_info.name += char(entry);
    }
  
    for (size_t i = 0; i < NINTENDO_LOGO.size(); i++) {
        if (m_memory.at(i + NINTENDO_LOGO_OFFSET) != NINTENDO_LOGO.at(i)) {
            return;
        }
    }

    m_info.type = BankType(m_memory.at(ROM_TYPE_OFFSET));

    m_bank = unique_ptr<MemoryBank>(initMemoryBank(m_info.type));

    assert(m_bank);
    
    LOG("ROM Size:  0x%x (0x%02x)\n", m_info.size, m_memory.at(ROM_SIZE_OFFSET));
    LOG("Bank Type: %s (0x%02x)\n", m_bank->name().c_str(), uint8_t(m_info.type));
    m_valid = true;
}

Cartridge::MemoryBank *Cartridge::initMemoryBank(uint8_t type)
{
    MemoryBank *bank = nullptr;

    switch (type) {
    default: 
        LOG("%s\n", "WARNING : unknown memory bank type");
        [[gnu::fallthrough]];

    case MBC_1:
    case MBC_1R:
    case MBC_1RB:
        bank = new MBC1(*this);
        break;

    case MBC_3RB:
        bank = new MBC3(*this);
        break;
    }

    return bank;
}

uint8_t & Cartridge::read(uint16_t address)
{
    assert(m_bank);

    if (address < (ROM_0_OFFSET + ROM_0_SIZE)) {
        return m_memory[address];
    }
    if (address < (ROM_1_OFFSET + ROM_1_SIZE)) {
        return m_bank->readROM(address);
    }
    if ((address >= EXT_RAM_OFFSET) && (address < (EXT_RAM_OFFSET + EXT_RAM_SIZE))) {
        return m_bank->readRAM(address);
    }

    assert(0);
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
void Cartridge::MemoryBank::writeRAM(uint16_t address, uint8_t value)
{
    uint32_t index = (address - EXT_RAM_OFFSET) + (m_ramBank * EXT_RAM_SIZE);
    m_ram[index] = value;
}

uint8_t & Cartridge::MemoryBank::readROM(uint16_t address)
{
    uint32_t index = address + ((m_romBank - 1) * ROM_1_SIZE);
    return m_cartridge.m_memory[index];
}

uint8_t & Cartridge::MemoryBank::readRAM(uint16_t address)
{
    uint32_t index = (address - EXT_RAM_OFFSET) + (m_ramBank * EXT_RAM_SIZE);
    return m_ram[index];
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Cartridge::MBC1::MBC1(Cartridge & cartridge)
    : MemoryBank(cartridge, "MBC1", 0x8000),
      m_mode(Cartridge::MBC_ROM)
{

}

void Cartridge::MBC1::writeROM(uint16_t address, uint8_t value)
{
    if (address < 0x2000) {
        m_ramEnable = ((value & 0x0F) == 0x0A);
    } else if (address < 0x4000) {
        uint8_t bank = (value & 0x1F);
        if ((0x00 == bank) || (0x20 == bank) || (0x40 == bank) || (0x60 == bank)) {
            bank |= 0x01;
        }

        m_romBank = ((m_romBank & 0xE0) | bank);
    } else if (address < 0x6000) {
        if (MBC_ROM == m_mode) {
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
Cartridge::MBC3::MBC3(Cartridge & cartridge)
    : MemoryBank(cartridge, "MBC3", 0x8000)
{

}

void Cartridge::MBC3::writeROM(uint16_t address, uint8_t value)
{
    // TODO: finsih the RTC portion of the ROM writing portion
    
    if (address < 0x2000) {
        m_ramEnable = ((value & 0x0F) == 0x0A);
    } else if (address < 0x4000) {
        uint8_t bank = (value & 0x7F);
        if (0x00 == bank) { bank |= 0x01; }

        m_romBank = bank;
    } else if (address < 0x6000) {
        if (value <= 0x03) {
            m_ramBank = value;
        } else {

        }
    } else if (address < 0x8000) {

    } else {
        LOG("Illegal MBC3 write to address 0x%04x\n", address);
        
        assert(0);
    }
}
////////////////////////////////////////////////////////////////////////////////



