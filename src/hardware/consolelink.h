#ifndef _CONSOLE_LINK_H
#define _CONSOLE_LINK_H

#include <cstdint>
#include <list>
#include <thread>
#include <atomic>
#include <utility>

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

    virtual void stop();

    void cycle(uint8_t ticks);
    void transfer(uint8_t value);

protected:
    static constexpr uint16_t LINK_SPEED_NORMAL = 4096;
    static constexpr uint16_t LINK_SPEED_FAST   = 128;

    MemoryController & m_memory;

    bool m_master;

    uint16_t m_ticks;
    uint32_t m_poll;

    std::atomic<bool> m_interrupt;
    std::atomic<bool> m_connected;

    struct {
        struct {
            std::atomic<bool> ready;
            std::atomic<uint8_t> data;
        } tx;
        struct {
            std::atomic<bool> ready;
            std::atomic<uint8_t> data;
        } rx;
    } m_queue;

    std::thread m_server;
    std::thread m_client;

    bool serverLoop();

    virtual bool rd(uint8_t & data) = 0;
    virtual bool wr(uint8_t data) = 0;

    virtual int peek() = 0;

private:
    static constexpr uint8_t SIMULATED_LINK_RESPONSE = 0xFF;

    static constexpr uint32_t POLL_COUNT = 120 * 1000;

    enum LinkState {
        STATE_IDLE,
        STATE_PENDING,
        STATE_DISCONNECTED,
        STATE_SIM_RESPONSE,
    };

    LinkState m_state;

    void check();

    void finishTransfer(uint8_t value);

    void handleIdle();
    void handlePending();
};

#endif
