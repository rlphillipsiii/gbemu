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

#include "gameboy.h"
#include "memmap.h"
#include "logging.h"
#include "profiler.h"

using std::string;
using std::vector;
using std::ifstream;
using std::chrono::high_resolution_clock;
using std::unique_lock;
using std::mutex;
using std::lock_guard;

GameBoy::GameBoy()
    : m_memory(*this),
      m_gpu(m_memory),
      m_cpu(m_memory),
      m_joypad(m_memory),
      m_run(false)
{
}

bool GameBoy::load(const string & filename)
{
    m_memory.setCartridge(filename);

    m_assembly = m_cpu.disassemble();
#if 0
    for (const Processor::Command & cmd : m_assembly) {
        std::cout << cmd.str() << std::endl;
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
    uint8_t ticks = m_cpu.cycle();

    m_gpu.cycle(ticks);
    m_joypad.cycle(ticks);
}
