/*
 * gameboy.cpp
 *
 *  Created on: May 28, 2019
 *      Author: Robert Phillips III
 */

#include <string>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>
#include <mutex>
#include <memory>

#include "gameboy.h"
#include "memmap.h"
#include "logging.h"
#include "socketlink.h"
#include "configuration.h"

using std::string;
using std::vector;
using std::unique_lock;
using std::mutex;
using std::unique_ptr;
using std::lock_guard;
using std::chrono::milliseconds;

using namespace std::chrono_literals;

GameBoy::GameBoy()
    : m_memory(*this),
      m_gpu(m_memory),
      m_cpu(*this),
      m_joypad(m_memory),
      m_ready(false),
      m_runCpu(false),
      m_runTimer(false),
      m_pauseCpu(false),
      m_pauseTimer(false),
      m_timerPaused(true),
      m_cpuPaused(true),
      m_ticks(0)
{
    initLink();
    readSpeed();

    Configuration::instance().registerListener(*this);
}

GameBoy::~GameBoy()
{
    Configuration::instance().unregisterListener(*this);
}

void GameBoy::readSpeed()
{
    EmuSpeed speed = Configuration::getEnum<EmuSpeed>(ConfigKey::SPEED);
    switch (speed) {
    default:
        assert(0);
        [[fallthrough]];

    case EmuSpeed::NORMAL: m_speed = TICKS_NORMAL; break;
    case EmuSpeed::_2X:    m_speed = TICKS_DOUBLE; break;
    case EmuSpeed::FREE:   m_speed = TICKS_FREE;   break;
    }
}

void GameBoy::initLink()
{
    if (m_link) { m_link->stop(); }

    if (Configuration::getBool(ConfigKey::LINK_ENABLE)) {
        m_link = unique_ptr<ConsoleLink>([&]() -> ConsoleLink* {
                LinkType link = Configuration::getEnum<LinkType>(ConfigKey::LINK_TYPE);
                switch (link) {
                default:
                    assert(0);
                    return nullptr;

                case LinkType::SOCKET:
                    return new SocketLink(m_memory);

                case LinkType::PIPE:
                    WARN("%s\n", "Pipe Link not yet supported");
                    return nullptr;
                }
            }());
    }
}

bool GameBoy::load(const string & filename)
{
    NOTE("Loading Rom: %s\n", filename.c_str())

    m_memory.setCartridge(filename);

    m_assembly = m_cpu.disassemble();
#if 0
    for (size_t i = 0; i < m_assembly.size(); ++i) {
        printf("0x%04x | %s\n", uint(i + 0x100), m_assembly.at(i).abbrev().c_str());
    }
#endif
    return true;
}

void GameBoy::start()
{
    m_runCpu.store(true, std::memory_order_release);
    m_runTimer.store(true, std::memory_order_release);

    m_thread = std::thread([&] { run(); });
    m_timer  = std::thread([&] { executeTimer(); });
}

void GameBoy::run()
{
    LOG("%s\n", "Gameboy thread running");

    m_cpu.reset();

    while (m_runCpu.load(std::memory_order_acquire)) {
        step();
    }
}

void GameBoy::stop()
{
    // Resume the CPU and timer just in case we happened to be paused.
    // We won't be able to stop the threads if they are paused.
    resume();

    m_runCpu.store(false, std::memory_order_release);

    if (m_link) {
        m_link->stop();
    }

    // Make sure that the CPU thread has stopped and exited before
    // attempting to stop the timer thread.  The timer thread controls
    // the execution timing of the CPU, so we need to guarantee that
    // the CPU thread is done before we start messing with the timer.
    if (m_thread.joinable()) { m_thread.join(); }

    m_runTimer.store(false, std::memory_order_release);
    if (m_timer.joinable()) { m_timer.join();  }
}

void GameBoy::step()
{
    while (m_pauseCpu.load(std::memory_order_acquire)) {
        if (!m_cpuPaused.load()) {
            m_cpuPaused.store(true, std::memory_order_release);
        }
        std::this_thread::sleep_for(5ms);
    }

    if (m_cpuPaused.load()) {
        m_cpuPaused.store(false, std::memory_order_release);
    }

    m_cpu.cycle();
}

void GameBoy::wait()
{
    unique_lock<mutex> lock(m_lock);
    m_cv.wait(lock, [&] { return m_ready; });

    m_ready = false;
}

void GameBoy::execute(uint8_t ticks)
{
    // Check to see if we have executed the number of ticks corresponding
    // to the timeout interval of the timer thread.  If so, we need to wait
    // here until the timer thread signals that we can continue to execute.
    // The one exception is "free" mode.  In that case, just let the thread
    // free run without any syncronization with the timer and skip this
    // check.
    m_ticks += ticks;
    if ((TICKS_FREE != m_speed) && (m_ticks >= m_speed)) {
        wait();

        m_ticks %= m_speed;
    }

    m_cpu.updateTimer(ticks);

    m_gpu.cycle(ticks);
    m_joypad.cycle(ticks);

    if (m_link) {
        m_link->cycle(ticks);
    }
}

void GameBoy::pause()
{
    // Make sure that the CPU is paused before pausing the timer thread
    // because the timer thread controls the execution timing of the
    // CPU thread, so we can't mess with the timer until the CPU thread
    // is paused.
    m_pauseCpu.store(true, std::memory_order_release);
    while (!m_cpuPaused.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(5ms);
    }

    // Block until the timer is paused.
    m_pauseTimer.store(true, std::memory_order_release);
    while (!m_timerPaused.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(5ms);
    }
}

void GameBoy::resume()
{
    m_ticks = 0;

    m_pauseCpu.store(false, std::memory_order_release);
    m_pauseTimer.store(false, std::memory_order_release);
}

void GameBoy::executeTimer()
{
    while (m_runTimer.load(std::memory_order_acquire)) {
        while (m_pauseTimer.load(std::memory_order_acquire)) {
            if (!m_timerPaused.load()) {
                m_timerPaused.store(true, std::memory_order_release);
            }
            std::this_thread::sleep_for(5ms);
        }

        if (m_timerPaused.load()) {
            m_timerPaused.store(false, std::memory_order_release);
        }

        // Put this thread to sleep.  When we wake up, then we will
        // notify the CPU thread that it's ok to continue.
        std::this_thread::sleep_for(milliseconds(REFRESH_MS));

        lock_guard<mutex> guard(m_lock);

        m_ready = true;
        m_cv.notify_one();
    }
}

void GameBoy::onConfigChange(ConfigKey key)
{
    // Config change updates run in the UI thread's context, so we need to make
    // sure that we pause the execution of these two threads before we make any
    // changes to the configuration.
    pause();

    switch (key) {
    case ConfigKey::SPEED: {
        readSpeed();
        break;
    }

    case ConfigKey::LINK_PORT:
    case ConfigKey::LINK_ADDR: {
        // Check the link type to see if we're changing a setting that is going
        // to apply to the active configuration.  If not, we can don't need to
        // do anything, so just break here.  Otherwise, fall through and restart
        // the interface.
        LinkType link = Configuration::getEnum<LinkType>(ConfigKey::LINK_TYPE);
        if (LinkType::SOCKET != link) {
            break;
        }
        [[fallthrough]];
    }

    case ConfigKey::LINK_MASTER:
    case ConfigKey::LINK_TYPE:
    case ConfigKey::LINK_ENABLE: {
        if (m_link) {
            m_link->stop();
            m_link.reset();
        }

        if (Configuration::getBool(key)) {
            initLink();
        }
        break;
    }

    default: break;
    }

    resume();
}
