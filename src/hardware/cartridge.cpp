#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>

#include "cartridge.h"
#include "logging.h"

using std::vector;
using std::string;
using std::ifstream;

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
      m_empty(0x00)
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

    m_info.mbc.type  = BankType(m_memory.at(ROM_TYPE_OFFSET));
    m_info.mbc.ramEn = false;
    m_info.mbc.bank  = 0x01;
    m_info.mbc.mode  = MBC_ROM;
  
    for (size_t i = 0; i < NINTENDO_LOGO.size(); i++) {
        if (m_memory.at(i + NINTENDO_LOGO_OFFSET) != NINTENDO_LOGO.at(i)) {
            return;
        }
    }

    LOG("ROM Size:  0x%x (0x%02x)\n", m_info.size, m_memory.at(ROM_SIZE_OFFSET));
    LOG("Bank Type: 0x%02x\n", uint8_t(m_info.mbc.type));
    
    m_valid = true;
}

void Cartridge::write(uint16_t address, uint8_t value)
{
    if (address < (ROM_0_OFFSET + ROM_0_SIZE)) {
        writeROM(address, value);
    } else {

    }
}

void Cartridge::writeROM(uint16_t address, uint8_t value)
{
    if (address <= 0x1FFF) {
        m_info.mbc.ramEn = ((value & 0x0F) == 0x0A);
    } else if (address <= 0x3FFF) {
        uint8_t bank = (value & 0x1F);
        if ((0x00 == bank) || (0x20 == bank) || (0x40 == bank) || (0x60 == bank)) {
            bank |= 0x01;
        }

        m_info.mbc.bank = ((m_info.mbc.bank & 0xE0) | bank);
    } else if (address <= 0x5FFF) {
        if (MBC_ROM == m_info.mbc.mode) {
            m_info.mbc.bank = (((value << 5) & 0xE0) | m_info.mbc.bank);
        } else {
            m_info.mbc.bank = (value & 0x07);
        }
    } else if (address <= 0x7FFF) {
        m_info.mbc.mode = (0x00 == value) ? MBC_ROM : MBC_RAM;
    } else {
        assert(0);
    }
}

void Cartridge::writeRAM(uint16_t address, uint8_t value)
{
    (void)address; (void)value;
}


