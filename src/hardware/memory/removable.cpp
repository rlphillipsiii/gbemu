#include <memory>
#include <string>

#include "removable.h"
#include "cartridge.h"
#include "memmap.h"

using std::string;

uint8_t Removable::EMPTY = 0xFF;

void Removable::load(const string & filename)
{
    m_cartridge = std::make_unique<Cartridge>(filename);
}

bool Removable::isValid() const
{
    return (m_cartridge) ? m_cartridge->isValid() : false;
}

void Removable::write(uint16_t address, uint8_t value)
{
    if (m_cartridge && m_cartridge->isValid()) {
        m_cartridge->write(address, value);
    }
}

uint8_t & Removable::ref(uint16_t address)
{
    return (m_cartridge && m_cartridge->isValid())
        ? m_cartridge->read(address) : EMPTY;
}

bool Removable::isAddressed(uint16_t address) const
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
