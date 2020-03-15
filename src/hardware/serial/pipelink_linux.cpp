#if 0

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <thread>
#include <mutex>
#include <cassert>

#include "pipelink_linux.h"
#include "memorycontroller.h"
#include "interrupt.h"
#include "logging.h"

using std::string;
using std::lock_guard;
using std::mutex;
using std::thread;

PipeLink::PipeLink(MemoryController & memory)
    : ConsoleLink(memory),
      m_pending(false),
      m_rFd(INVALID_FD),
      m_wFd(INVALID_FD),
      m_rOpen(false),
      m_wOpen(false),
      m_interrupt(false)
{
    mkfifo(MASTER_PIPE_TX, 0666);
    mkfifo(MASTER_PIPE_RX, 0666);

    NOTE("Pipe link configured as %s\n", (m_master) ? "master" : "slave");

    const char *rname = m_master ? MASTER_PIPE_RX : MASTER_PIPE_TX;
    const char *wname = m_master ? MASTER_PIPE_TX : MASTER_PIPE_RX;

    m_rx = thread([&] { rx(rname); });
    m_tx = thread([&] { tx(wname); });
}

PipeLink::~PipeLink()
{
    m_interrupt = true;

    m_rx.join();
    m_tx.join();
}

void PipeLink::transfer(uint8_t value)
{
    uint8_t data = 0xFF;

    if (m_wOpen) {
        m_memory.write(SERIAL_DATA_ADDRESS, value);

        {
            lock_guard<mutex> guard(m_txLock);
            m_txQueue.push_back(value);
        }

        while (m_rOpen) {
            lock_guard<mutex> guard(m_rxLock);
            if (m_rxQueue.empty()) { continue; }

            data = m_rxQueue.front();
            m_rxQueue.pop_front();

            break;
        }
    }

    m_memory.write(SERIAL_DATA_ADDRESS, data);
    Interrupts::set(m_memory, InterruptMask::SERIAL);
}

void PipeLink::check()
{
#if 0
    if (!m_pending) { return; }

    uint8_t data;
    {
        lock_guard<mutex> guard(m_rxLock);
        if (m_rxQueue.empty()) { return; }

        data = m_rxQueue.back();
        m_rxQueue.clear();
    }

    m_memory.write(SERIAL_DATA_ADDRESS, data);
    Interrupts::set(m_memory, InterruptMask::SERIAL);

    m_pending = false;
#endif
}

void PipeLink::tx(const string & name)
{
    while (!m_interrupt.load()) {
        m_wFd = open(name.c_str(), O_WRONLY);

        m_wOpen = true;
        while (m_wOpen) {
            uint8_t data;
            {
                lock_guard<mutex> guard(m_txLock);
                if (m_txQueue.empty()) { continue; }

                data = m_txQueue.front();
                m_txQueue.pop_front();
            }

            printf("TX: 0x%02x\n", data);

            if (write(m_wFd, reinterpret_cast<void*>(&data), 1) == IO_ERROR) {
                m_wOpen = false;
            }
        }

        if (INVALID_FD != m_wFd) { close(m_wFd); }
    }


}

void PipeLink::rx(const string & name)
{
    while (!m_interrupt.load()) {
        m_rFd = open(name.c_str(), O_RDONLY);

        m_rOpen = true;
        while (m_rOpen) {
            uint8_t data;

            if (read(m_rFd, reinterpret_cast<void*>(&data), 1) == IO_ERROR) {
                m_rOpen = false;
            } else {
                if (!m_wOpen) { continue; }

                lock_guard<mutex> guard(m_rxLock);
                m_rxQueue.push_back(data);

                printf("RX: 0x%02x\n", data);
            }
        }

        if (INVALID_FD != m_rFd) { close(m_rFd); }
    }
}
#endif
