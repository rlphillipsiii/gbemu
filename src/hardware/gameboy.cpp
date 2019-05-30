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

#include "gameboy.h"
#include "memmap.h"

using std::string;
using std::vector;

GameBoy::GameBoy()
    : m_cpu(m_memory),
      m_gpu(m_memory)
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
    return (0 == result);
}
