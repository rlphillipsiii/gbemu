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
using std::lock_guard;
using std::unique_ptr;

GameBoy::GameBoy()
    : m_memory(*this),
      m_gpu(m_memory),
      m_cpu(*this),
      m_joypad(m_memory),
      m_run(false)
{
    initLink();

    Configuration::instance().registerListener(*this);
}

GameBoy::~GameBoy()
{
    Configuration::instance().unregisterListener(*this);
}

void GameBoy::initLink()
{
    if (m_link) { m_link->stop(); }

    if (Configuration::getBool(ConfigKey::LINK_ENABLE)) {
        m_link = unique_ptr<ConsoleLink>([&]() -> ConsoleLink* {
                LinkType link = LinkType(Configuration::getInt(ConfigKey::LINK_TYPE));
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
    m_run = true;

    m_thread = std::thread([&] { run(); });
}

void GameBoy::run()
{
    LOG("%s\n", "Gameboy thread running");

    m_cpu.reset();

    while (m_run.load()) {
        step();
    }
}

void GameBoy::stop()
{
    m_run = false;

    if (m_link) {
        m_link->stop();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void GameBoy::step()
{
    m_cpu.cycle();
}

void GameBoy::execute(uint8_t ticks)
{
    m_cpu.updateTimer(ticks);

    m_gpu.cycle(ticks);
    m_joypad.cycle(ticks);

    if (m_link) {
        m_link->cycle(ticks);
    }
}

void GameBoy::onConfigChange(ConfigKey key)
{
    switch (key) {
    case ConfigKey::LINK_PORT:
    case ConfigKey::LINK_ADDR: {
        // Check the link type to see if we're changing a setting that is going
        // to apply to the active configuration.  If not, we can don't need to
        // do anything, so just break here.  Otherwise, fall through and restart
        // the interface.
        LinkType link = LinkType(Configuration::getInt(ConfigKey::LINK_TYPE));
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
