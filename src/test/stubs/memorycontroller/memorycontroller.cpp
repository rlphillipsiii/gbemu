#include "memorycontroller.h"

void MemoryController::write(uint16_t address, uint8_t value)
{
    m_memory[address] = value;
}

uint8_t & MemoryController::read(uint16_t address)
{
    return m_memory[address];
}

const uint8_t & MemoryController::peek(uint16_t address)
{
    return read(address);
}
