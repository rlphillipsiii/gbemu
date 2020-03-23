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
    m_runCpu   = true;
    m_runTimer = true;

    m_thread = std::thread([&] { run(); });
    m_timer  = std::thread([&] { executeTimer(); });
}

void GameBoy::run()
{
    LOG("%s\n", "Gameboy thread running");

    m_cpu.reset();

    while (m_runCpu) {
        step();
    }
}

void GameBoy::stop()
{
    m_runCpu = false;

    if (m_link) {
        m_link->stop();
    }

    if (m_thread.joinable()) { m_thread.join(); }

    m_runTimer = false;
    if (m_timer.joinable())  { m_timer.join();  }
}

void GameBoy::step()
{
    while (m_pauseCpu) {
        m_cpuPaused = true;
        std::this_thread::sleep_for(100ms);
    }

    m_cpuPaused = false;

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
    m_ticks += ticks;
    while ((TICKS_FREE != m_speed) && (m_ticks >= m_speed)) {
        wait();

        m_ticks -= m_speed;
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
    m_pauseCpu = true;
    while (!m_cpuPaused)   { std::this_thread::sleep_for(100ms); }

    m_pauseTimer = true;
    while (!m_timerPaused) { std::this_thread::sleep_for(100ms); }
}

void GameBoy::resume()
{
    m_pauseCpu   = false;
    m_pauseTimer = false;
}

void GameBoy::executeTimer()
{
    while (m_runTimer) {
        while (m_pauseTimer) {
            m_cpuPaused = true;
            std::this_thread::sleep_for(100ms);
        }

        m_cpuPaused = false;

        std::this_thread::sleep_for(milliseconds(REFRESH_MS));

        lock_guard<mutex> guard(m_lock);

        m_ready = true;
        m_cv.notify_one();
    }
}

void GameBoy::onConfigChange(ConfigKey key)
{
    switch (key) {
    case ConfigKey::SPEED: {
        pause();
        readSpeed();
        resume();

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
}
