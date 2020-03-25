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
#include <sstream>
#include <iomanip>

#include "gbrgb.h"

class GameBoyInterface {
public:
    static constexpr int WIDTH  = 160;
    static constexpr int HEIGHT = 144;


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

    static std::shared_ptr<GameBoyInterface> Instance(ConsoleType type=GAMEBOY_EMU);

    virtual ~GameBoyInterface() = default;

    virtual bool load(const std::string & filename) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void setButton(JoyPadButton button) = 0;
    virtual void clrButton(JoyPadButton button) = 0;

    virtual ColorArray && getRGB() = 0;
};

#endif /* GAMEBOYINTERFACE_H_ */
