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

#include "gameboy.h"
#include "memmap.h"

using std::string;
using std::vector;
using std::ifstream;
using std::chrono::high_resolution_clock;

const uint16_t GameBoy::ROM_HEADER_LENGTH   = 0x0180;
const uint16_t GameBoy::ROM_NAME_OFFSET     = 0x0134;
const uint16_t GameBoy::ROM_TYPE_OFFSET     = 0x0147;
const uint16_t GameBoy::ROM_SIZE_OFFSET     = 0x0148;
const uint16_t GameBoy::ROM_RAM_SIZE_OFFSET = 0x0149;

const uint8_t GameBoy::ROM_NAME_MAX_LENGTH = 0x10;

GameBoy::GameBoy()
    : m_cpu(m_memory),
      m_gpu(m_memory),
      m_run(false)
{
    m_memory.setGPU(&m_gpu);
}

bool GameBoy::load(const string & filename)
{
    ifstream input(filename, std::ios::in | std::ios::binary);
    if (!input.is_open()) { return false; }

    input.seekg(0, input.end);
    size_t length = input.tellg();
    input.seekg(0, input.beg);

    if (length < ROM_HEADER_LENGTH) {
        return false;
    }
    
    vector<uint8_t> data(length);
    input.read(reinterpret_cast<char*>(data.data()), data.size());

    RomHeader info = parseHeader(data);
    printf("ROM name: %s\n", info.name.c_str());

    for (size_t i = 0; i < data.size(); i++) {
        m_memory.write(ROM_0_OFFSET + i, data.at(i));
    }

    // return (0 == result);
    return true;
}

GameBoy::RomHeader GameBoy::parseHeader(const vector<uint8_t> & header)
{
    RomHeader info;

    for (uint8_t i = 0; i < ROM_NAME_MAX_LENGTH; i++) {
        uint8_t entry = header.at(ROM_NAME_OFFSET + i);

        if ((0x80 == entry) || (0xC0 == entry)) {
            break;
        }

        info.name += char(entry);
    }

    uint8_t size = header.at(ROM_SIZE_OFFSET);
    
    return info;
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

            // std::cout << useconds << std::endl;
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
