#include <thread>
#include <cassert>

#include "consolelink.h"
#include "configuration.h"
#include "memorycontroller.h"
#include "logging.h"
#include "interrupt.h"

using std::unique_lock;
using std::lock_guard;
using std::mutex;

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
    uint8_t recv = 0xFF;

    bool empty = true;
    while (empty) {
        if (!m_connected) { break; }

        unique_lock<mutex> guard(m_queue.rx.lock);

        if ((empty = m_queue.rx.data.empty())) {
            guard.unlock();

            std::this_thread::sleep_for(1ms);
            continue;
        }

        recv = m_queue.rx.data.front();
        m_queue.rx.data.pop_front();
    }

    finishTransfer(recv);
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

        lock_guard<mutex> guard(m_queue.tx.lock);
        m_queue.tx.data.push_back(data);
    } else {
        m_state = STATE_SIM_RESPONSE;
    }
}

bool ConsoleLink::serverLoop()
{
    uint8_t data = 0xFF;
    {
        lock_guard<mutex> guard(m_queue.tx.lock);
        if (m_queue.tx.data.empty()) { return true; }

        data = m_queue.tx.data.front();
        m_queue.tx.data.pop_front();
    }

    if (!wr(data)) { return false; }
    if (!rd(data)) { return false; }

    lock_guard<mutex> guard(m_queue.rx.lock);
    m_queue.rx.data.push_back(data);

    return true;
}
