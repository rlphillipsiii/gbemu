#include <thread>
#include <cassert>

#include "consolelink.h"
#include "configuration.h"
#include "memorycontroller.h"
#include "logging.h"
#include "interrupt.h"

using namespace std::chrono_literals;

ConsoleLink::ConsoleLink(MemoryController & memory)
    : m_memory(memory),
      m_ticks(0),
      m_poll(0),
      m_interrupt(false),
      m_connected(false),
      m_state(ConsoleLink::STATE_DISCONNECTED)
{
    m_master = Configuration::getBool(ConfigKey::LINK_MASTER);

    m_queue.tx.ready = false;
    m_queue.rx.ready = false;
}

void ConsoleLink::stop()
{
    m_interrupt = true;

    if (m_server.joinable()) { m_server.join(); }
    if (m_client.joinable()) { m_client.join(); }
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

void ConsoleLink::check()
{
    switch (m_state) {
    default:
        assert(0);
        [[fallthrough]];

    case STATE_IDLE:
    case STATE_DISCONNECTED:
        break;

    case STATE_SIM_RESPONSE:
        finishTransfer(SIMULATED_LINK_RESPONSE);
        break;

    case STATE_PENDING:
        handlePending();
        break;
    }
}

void ConsoleLink::handlePending()
{
    while (!m_queue.rx.ready.load(std::memory_order_acquire)) {
        if (!m_connected) { break; }

        std::this_thread::sleep_for(1ms);
    }

    m_queue.rx.ready.store(false, std::memory_order_release);

    finishTransfer(m_queue.rx.data.load(std::memory_order_acquire));
}

void ConsoleLink::finishTransfer(uint8_t data)
{
    m_memory.write(SERIAL_DATA_ADDRESS, data);

    uint8_t & current = m_memory.read(SERIAL_CONTROL_ADDRESS);
    current &= ~ConsoleLink::LINK_TRANSFER;

    Interrupts::set(m_memory, InterruptMask::SERIAL);

    m_state =
        (STATE_PENDING == m_state) ? STATE_IDLE : STATE_DISCONNECTED;
}

void ConsoleLink::transfer(uint8_t data)
{
    m_ticks = 0;

    if (m_connected) {
        m_state = STATE_PENDING;

        m_queue.tx.data.store(data, std::memory_order_release);
        m_queue.tx.ready.store(true, std::memory_order_release);
    } else {
        m_state = STATE_SIM_RESPONSE;
    }
}

bool ConsoleLink::serverLoop()
{
    if (!m_queue.tx.ready.load(std::memory_order_acquire)
        || m_queue.rx.ready.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(10ms);
        return true;
    }

    m_queue.tx.ready.store(false, std::memory_order_release);

    uint8_t data = m_queue.tx.data.load(std::memory_order_acquire);
    if (!wr(data)) { return false; }
    if (!rd(data)) { return false; }

    m_queue.rx.data.store(data, std::memory_order_release);
    m_queue.rx.ready.store(true, std::memory_order_release);

    return true;
}
