#include "consolelink.h"
#include "configuration.h"
#include "memorycontroller.h"

ConsoleLink::ConsoleLink(MemoryController & memory)
    : m_memory(memory),
      m_ticks(0)
{
    Configuration & config = Configuration::instance();

    Configuration::Setting master = config[ConfigKey::LINK_MASTER];
    m_master = (master) ? master->toBool() : true;
}

void ConsoleLink::cycle(uint8_t ticks)
{
    // TODO: Need to handle CGB high speed mode here, which should just shift
    //       these values down by 1 (i.e. double the speed)
    const uint16_t count = (m_memory.read(SERIAL_CONTROL_ADDRESS) & LINK_SPEED) ?
        LINK_SPEED_FAST : LINK_SPEED_NORMAL;

    m_ticks += ticks;
    while (m_ticks >= count) {
        m_ticks -= count;

        check();
    }
}

void ConsoleLink::transfer(uint8_t data)
{
    m_ticks = 0;

    start(data);
}
