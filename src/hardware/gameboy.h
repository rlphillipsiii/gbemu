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
#include <memory>

#include "gameboyinterface.h"
#include "gpu.h"
#include "memorycontroller.h"
#include "processor.h"
#include "joypad.h"
#include "consolelink.h"

class GameBoy : public GameBoyInterface {
public:
    GameBoy();
    ~GameBoy() = default;

    bool load(const std::string & filename) override;

    void start() override;
    void stop() override;

    void execute(uint8_t ticks);

    inline void setButton(JoyPadButton button) override { m_joypad.set(button); }
    inline void clrButton(JoyPadButton button) override { m_joypad.clr(button); }

    inline ColorArray && getRGB() override { return m_gpu.getColorMap(); }

    inline GPU & gpu() { return m_gpu; }
    inline Processor & cpu() { return m_cpu; }
    inline MemoryController & mmc() { return m_memory; }
    inline JoyPad & joypad() { return m_joypad; }
    inline std::unique_ptr<ConsoleLink> & link() { return m_link; }

private:
    MemoryController m_memory;
    GPU m_gpu;
    Processor m_cpu;
    JoyPad m_joypad;
    std::unique_ptr<ConsoleLink> m_link;

    std::atomic<bool> m_run;

    std::thread m_thread;

    std::vector<Processor::Command> m_assembly;

    void run();
    void step();
};



#endif /* GAMEBOY_H_ */
