#include "debugger.h"
#include "gameboy.h"
#include "processor.h"
#include "logging.h"
#include "memmap.h"

Debugger::Debugger(GameBoy & gameboy, Processor & cpu)
    : m_gameboy(gameboy),
      m_cpu(cpu)
{
    //m_breakpoints.insert(0x0040);
    //m_access[0x8080] = GB::MEM_ACCESS_WRITE;
    m_access[0xCF67] = GB::MEM_ACCESS_WRITE;
}

void Debugger::check()
{
    if (!m_breakpoints.empty()) {
        // TODO: should probably move this logic to the CPU
        uint16_t pc = m_cpu.pc();
        if (m_cpu.interruptsEnabled()) {
            auto vect = m_cpu.checkInterrupts();
            if (InterruptVector::INVALID != vect) {
                pc = uint8_t(vect);
            }
        }

        auto iterator = m_breakpoints.find(pc);
        if (m_breakpoints.end() != iterator) {
            m_gameboy.pause();

            LOG("Breakpoint hit: 0x%04x\n", *iterator);
        }
    }

    while (!m_memQ.empty()) {
        auto [type, address] = m_memQ.front();
        m_memQ.pop();

        auto iterator = m_access.find(address);
        if (m_access.end() != iterator) {
            GB::MemAccessType request = iterator->second;
            if ((request == type) || (GB::MEM_ACCESS_EITHER == request)) {
                m_gameboy.pause();
            }
        }
    }
}
