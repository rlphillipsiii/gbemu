/*
 * gameboyinterface.h
 *
 *  Created on: Jun 19, 2019
 *      Author: Robert Phillips III
 */

#ifndef GAMEBOYINTERFACE_H_
#define GAMEBOYINTERFACE_H_

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <stack>
#include <utility>

#include "gbrgb.h"
#include "gbproc.h"

class GameBoyInterface {
public:
    static constexpr const int WIDTH  = 160;
    static constexpr const int HEIGHT = 144;

    static std::unique_ptr<GameBoyInterface> NULLOPT;

    enum ConsoleType {
        GAMEBOY_EMU,
    };

    enum JoyPadButton {
        JOYPAD_A      = 0x10,
        JOYPAD_B      = 0x20,
        JOYPAD_SELECT = 0x40,
        JOYPAD_START  = 0x80,
        JOYPAD_RIGHT  = 0x01,
        JOYPAD_LEFT   = 0x02,
        JOYPAD_UP     = 0x04,
        JOYPAD_DOWN   = 0x08,
    };

    static std::unique_ptr<GameBoyInterface> Instance(ConsoleType type=GAMEBOY_EMU);

    virtual ~GameBoyInterface() = default;

    virtual bool load(const std::string & filename) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void step() = 0;

    virtual void pause() = 0;
    virtual void resume() = 0;

    virtual bool paused() const = 0;

    virtual void setBreakpoint(uint16_t address) = 0;
    virtual void clrBreakpoint(uint16_t address) = 0;

    virtual void setButton(JoyPadButton button) = 0;
    virtual void clrButton(JoyPadButton button) = 0;

    virtual const std::list<GB::Command> & trace() const = 0;

    virtual std::stack<GB::Command> callstack() const = 0;

    virtual std::pair<int, ColorArray&&> getRGB() = 0;

    virtual void write(uint16_t address, uint8_t value) = 0;
    virtual uint8_t read(uint16_t address) = 0;

    class Halt final {
    public:
        explicit Halt(GameBoyInterface & gameboy)
            : m_gameboy(gameboy) { m_gameboy.pause(); }
        ~Halt() { m_gameboy.resume(); }
    private:
        GameBoyInterface & m_gameboy;
    };
};

using GameBoyReference = std::reference_wrapper<std::unique_ptr<GameBoyInterface>>;

#endif /* GAMEBOYINTERFACE_H_ */
