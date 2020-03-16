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

    virtual void stop() = 0;

    void cycle(uint8_t ticks);
    void transfer(uint8_t value);

protected:
    static constexpr uint16_t LINK_SPEED_NORMAL = 4096;
    static constexpr uint16_t LINK_SPEED_FAST   = 128;

    MemoryController & m_memory;

    bool m_master;

    uint16_t m_ticks;

    virtual void check() = 0;
    virtual void start(uint8_t value) = 0;
};

#endif
