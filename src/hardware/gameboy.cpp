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
    : m_cpu(m_memory),
      m_gpu(m_memory),
      m_joypad(m_memory),
      m_run(false)
{
#ifdef STATIC_MEMORY
    ifstream ram("memory2.bin", std::ios::in | std::ios::binary);
    //for (int i = GPU_RAM_OFFSET; i <= 0xFFFF; i++) {
    for (int i = 0; i <= 0xFFFF; i++) {
        uint8_t b;
        ram.read(reinterpret_cast<char*>(&b), 1);
        
        m_memory.initialize(i, b);
    }
    std::cout << "Memory controller initialized" << std::endl;
#endif
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

#ifndef STATIC_MEMORY
    m_thread = std::thread([this]() {
        this->run();
    });
#endif
}

void GameBoy::run()
{
    LOG("%s\n", "Gameboy thread running");

    const int CYCLES = 120000;
    
    int count = 0;
    
    while (m_run.load()) {
        {
            TimeProfiler p("Clock Cycle Loop");
            while (count < CYCLES) {
                uint8_t ticks = m_cpu.cycle();

                m_gpu.cycle(ticks);

                m_joypad.cycle(ticks);

                count += ticks;
            }
        }

        count =- CYCLES;

#ifdef PROFILING
        string temp;
        std::getline(std::cin, temp);
#endif
    }
}

void GameBoy::stop()
{
    m_run = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

