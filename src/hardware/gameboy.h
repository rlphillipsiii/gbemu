/*
 * gameboy.h
 *
 *  Created on: May 28, 2019
 *      Author: Robert Phillips III
 */

#ifndef GAMEBOY_H_
#define GAMEBOY_H_

#include <cstdint>
#include <string>
#include <atomic>
#include <thread>
#include <vector>

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

    std::vector<GPU::RGB> getRGB() { return m_gpu.lookup(GPU::MAP_0); }
    
private:
    static const uint16_t ROM_HEADER_LENGTH;
    static const uint16_t ROM_NAME_OFFSET;
    static const uint16_t ROM_TYPE_OFFSET;
    static const uint16_t ROM_SIZE_OFFSET;
    static const uint16_t ROM_RAM_SIZE_OFFSET;
    static const uint8_t ROM_NAME_MAX_LENGTH;
    
    struct RomHeader {
        std::string name;
        
        uint16_t length;
    };
    
    MemoryController m_memory;
    Processor m_cpu;
    GPU m_gpu;

    std::atomic<bool> m_run;

    std::thread m_thread;

    void run();
    RomHeader parseHeader(const std::vector<uint8_t> & header);
};



#endif /* GAMEBOY_H_ */
