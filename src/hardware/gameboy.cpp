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
#include <iostream>
#include <fstream>
#include <mutex>
#include <memory>

#include "gameboy.h"
#include "memmap.h"
#include "logging.h"
#include "socketlink.h"

using std::string;
using std::vector;
using std::ifstream;
using std::chrono::high_resolution_clock;
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
    m_link = unique_ptr<ConsoleLink>(new SocketLink(m_memory));
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

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void GameBoy::step()
{
    if (m_link) {
        m_link->check();
    }
    m_cpu.cycle();
}

void GameBoy::execute(uint8_t ticks)
{
    m_cpu.updateTimer(ticks);

    m_gpu.cycle(ticks);
    m_joypad.cycle(ticks);
}
