#include "debugger.h"
#include "gameboy.h"
#include "processor.h"
#include "logging.h"

Debugger::Debugger(GameBoy & gameboy, Processor & cpu)
    : m_gameboy(gameboy),
      m_cpu(cpu)
{
    m_access[0xCFB3] = GB::MEM_ACCESS_WRITE;
}

void Debugger::check()
{
    if (!m_breakpoints.empty()) {
        auto iterator = m_breakpoints.find(m_cpu.pc());
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
                printf("%s\n", m_gameboy.trace().back().str().c_str());
            }
        }
    }
}
