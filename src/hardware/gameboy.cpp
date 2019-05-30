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

#include "gameboy.h"
#include "memmap.h"

using std::string;
using std::vector;
using std::chrono::high_resolution_clock;

GameBoy::GameBoy()
    : m_cpu(m_memory),
      m_gpu(m_memory),
      m_run(false)
{
    m_memory.setGPU(&m_gpu);
}

bool GameBoy::load(const string & filename)
{
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) { return false; }

#if 0
    uint16_t address = ROM_0_OFFSET;

    ssize_t result;
    while (true) {
        uint8_t & memory = m_memory.read(address++);

        result = fread(&memory, 1, 1, file);
        if (0 >= result) {
            break;
        }
    }
#endif

    fclose(file);
    // return (0 == result);
    return true;
}

void GameBoy::start()
{
    m_run = true;

    m_thread = std::thread([this]() {
        this->run();
    });
}

void GameBoy::run()
{
    auto start = high_resolution_clock::now();

    while (m_run.load()) {
        auto end = high_resolution_clock::now();

        int useconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (useconds >= 25) {
            m_cpu.cycle();

            std::cout << useconds << std::endl;
        }

        start = high_resolution_clock::now();
    }
}

void GameBoy::stop()
{
    m_run = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }
}
