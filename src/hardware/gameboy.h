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
#include <mutex>
#include <condition_variable>

#include "gameboyinterface.h"
#include "gpu.h"
#include "memorycontroller.h"
#include "processor.h"
#include "joypad.h"

class GameBoy : public GameBoyInterface {
public:
    GameBoy();
    ~GameBoy() = default;

    bool load(const std::string & filename) override;

    void start() override;
    void stop() override;

    inline void setButton(JoyPadButton button) override { m_joypad.set(button); }
    inline void clrButton(JoyPadButton button) override { m_joypad.clr(button); }

    inline ColorArray getRGB() override
    {
        std::lock_guard<std::mutex> lock(m_sync);
        return std::move(m_screen);
    }

private:
    MemoryController m_memory;
    Processor m_cpu;
    GPU m_gpu;
    JoyPad m_joypad;

    ColorArray m_screen;
    
    std::condition_variable m_wait;
    std::mutex m_sync;
    
    std::atomic<bool> m_run;
    std::atomic<bool> m_advance;
    std::atomic<bool> m_running;
    
    std::thread m_thread;

    std::vector<Processor::Command> m_assembly;
    
    void run();
};



#endif /* GAMEBOY_H_ */
