/*
 * gameboy.h
 *
 *  Created on: May 28, 2019
 *      Author: Robert Phillips III
 */

#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include <string>

#include "gpu.h"
#include "memorycontroller.h"
#include "processor.h"

class GameBoy {
public:
    GameBoy();
    ~GameBoy() = default;

    bool load(const std::string & filename);

private:
    MemoryController m_memory;
    Processor m_cpu;
    GPU m_gpu;
};



#endif /* GAMEBOY_H_ */
