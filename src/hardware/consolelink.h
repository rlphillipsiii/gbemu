#ifndef _CONSOLE_LINK_H
#define _CONSOLE_LINK_H

#include <cstdint>

class MemoryController;

class ConsoleLink {
public:
    enum LinkAttributes {
        LINK_TRANSFER = 0x80,
        LINK_SPEED    = 0x02,
        LINK_CLOCK    = 0x01,
    };

    explicit ConsoleLink(MemoryController & memory);
    virtual ~ConsoleLink() = default;

    virtual void transfer(uint8_t value) = 0;
    virtual void check() = 0;

    virtual void stop() = 0;

protected:
    MemoryController & m_memory;

    bool m_master;
};

#endif
