/*
 * gameboy.h
 *
 *  Created on: May 28, 2019
 *      Author: Robert Phillips III
 */

#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include <string>
#include <atomic>
#include <thread>

#include "gpu.h"
#include "memorycontroller.h"
#include "processor.h"

class GameBoy {
public:
    GameBoy();
    ~GameBoy() = default;

    bool load(const std::string & filename);

    void start();
    void stop();

private:
    MemoryController m_memory;
    Processor m_cpu;
    GPU m_gpu;

    std::atomic<bool> m_run;

    std::thread m_thread;

    void run();
};



#endif /* GAMEBOY_H_ */
