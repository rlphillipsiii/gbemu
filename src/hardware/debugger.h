#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <queue>
#include <utility>

#include "gbproc.h"

class GameBoy;
class Processor;

class Debugger final {
public:
    explicit Debugger(GameBoy & gameboy, Processor & cpu);
    ~Debugger() = default;

    void check();

    inline void setBreakpoint(uint16_t address);
    inline void clrBreakpoint(uint16_t address);

    inline void enqueue(GB::MemAccessType type, uint16_t address);

private:
    GameBoy & m_gameboy;
    Processor & m_cpu;

    std::queue<std::pair<GB::MemAccessType, uint16_t>> m_memQ;

    std::unordered_set<uint16_t> m_breakpoints;
    std::unordered_map<uint16_t, GB::MemAccessType> m_access;
};

inline void Debugger::enqueue(GB::MemAccessType type, uint16_t address)
{
    m_memQ.push({ type, address });
}

inline void Debugger::setBreakpoint(uint16_t address)
{
    m_breakpoints.insert(address);
}

inline void Debugger::clrBreakpoint(uint16_t address)
{
    auto iterator = m_breakpoints.find(address);
    if (m_breakpoints.end() != iterator) {
        m_breakpoints.erase(iterator);
    }
}

#endif
