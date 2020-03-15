#include "consolelink.h"
#include "configuration.h"
#include "memorycontroller.h"

ConsoleLink::ConsoleLink(MemoryController & memory)
    : m_memory(memory)
{
    Configuration & config = Configuration::instance();

    Configuration::Setting master = config[ConfigKey::LINK_MASTER];
    m_master = (master) ? master->toBool() : true;
}
