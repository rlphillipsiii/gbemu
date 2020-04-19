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
#include <condition_variable>
#include <mutex>

#include "gameboyinterface.h"
#include "gpu.h"
#include "memorycontroller.h"
#include "processor.h"
#include "joypad.h"
#include "consolelink.h"
#include "configuration.h"
#include "clockinterface.h"
#include "debugger.h"
#include "memaccess.h"
#include "gbproc.h"

class GameBoy final : public GameBoyInterface, public ConfigChangeListener {
public:
    explicit GameBoy();
    ~GameBoy();

    bool load(const std::string & filename) override;

    void start() override;
    void stop() override;

    void pause() override;
    void resume() override;

    inline void setBreakpoint(uint16_t address) override
        { Halt h(*this); m_debugger.setBreakpoint(address); }
    inline void clrBreakpoint(uint16_t address) override
        { Halt h(*this); m_debugger.clrBreakpoint(address); }

    inline void write(uint16_t address, uint8_t value) override
        { Halt h(*this); m_memory.write(address, value); }
    inline uint8_t read(uint16_t address) override
        { Halt h(*this); return m_memory.peek(address); }

    inline const std::list<GB::Command> & trace() const override
        { return m_cpu.trace(); }

    void onConfigChange(ConfigKey key) override;

    inline void setButton(JoyPadButton button) override { m_joypad.set(button); }
    inline void clrButton(JoyPadButton button) override { m_joypad.clr(button); }

    inline ColorArray && getRGB() override { return m_gpu.getColorMap(); }

    inline GPU & gpu() { return m_gpu; }
    inline Processor & cpu() { return m_cpu; }
    inline MemoryController & mmc() { return m_memory; }
    inline JoyPad & joypad() { return m_joypad; }
    inline std::unique_ptr<ConsoleLink> & link() { return m_link; }

private:
    static constexpr uint32_t REFRESH_MS = 20;

    // The number of ticks per interval is the clock rate (~4MHz), divided
    // by the number refresh intervals per second (number of milliseconds
    // per second divided by the refresh rate)
    static constexpr uint32_t TICKS_NORMAL = 4e6 / (1e3 / REFRESH_MS);
    static constexpr uint32_t TICKS_DOUBLE = TICKS_NORMAL * 2;
    static constexpr uint32_t TICKS_QUAD   = TICKS_NORMAL * 4;
    static constexpr uint32_t TICKS_FREE   = 0;

    class Clock final : public ClockInterface {
    public:
        explicit Clock(GameBoy & gameboy)
            : m_hardware(gameboy), m_ticks(0) { }
        ~Clock() = default;

        void tick(uint8_t cycles) override;

        inline void setSpeed(uint32_t speed) { m_speed = speed; }
        inline void reset() { m_ticks = 0; }

    private:
        GameBoy & m_hardware;

        uint32_t m_ticks;
        std::atomic<uint32_t> m_speed;
    };

    Clock m_clock;

    class MemAccess final : public MemAccessListener {
    public:
        explicit MemAccess(GameBoy & gameboy)
            : m_hardware(gameboy) { }
        ~MemAccess() = default;

        void onMemoryAccess(
            GB::MemAccessType type, uint16_t address) override;

    private:
        GameBoy & m_hardware;
    };

    MemAccess m_memListener;

    MemoryController m_memory;
    GPU m_gpu;
    Processor m_cpu;
    JoyPad m_joypad;
    Debugger m_debugger;
    std::unique_ptr<ConsoleLink> m_link;

    std::condition_variable m_cv;
    std::mutex m_lock;

    bool m_ready;
    bool m_halt;

    std::atomic<bool> m_runCpu;
    std::atomic<bool> m_runTimer;

    std::atomic<bool> m_pauseCpu;
    std::atomic<bool> m_pauseTimer;

    std::atomic<bool> m_timerPaused;
    std::atomic<bool> m_cpuPaused;

    std::thread m_thread;
    std::thread m_timer;

    void run();
    void step();
    void wait();

    void initLink();
    void readSpeed();

    void executeTimer();
};



#endif /* GAMEBOY_H_ */
