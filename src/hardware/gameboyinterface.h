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

#define LCD_SCREEN_WIDTH  160
#define LCD_SCREEN_HEIGHT 144

namespace GB {
    struct RGB {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
            : red(r), green(g), blue(b), alpha(a) { }
              
        RGB() : RGB(0, 0, 0, 0) { }

        RGB(const RGB & other)
            : RGB(other.red, other.blue, other.green, other.alpha) { }

        RGB & operator=(const RGB & other)
        {
            this->red   = other.red;
            this->green = other.green;
            this->blue  = other.blue;
            this->alpha = other.alpha;
            return *this;
        }

        bool operator==(const RGB & other)
        {
            return ((this->red == other.red) && (this->green == other.green)
                    && (this->blue == other.blue) && (this->alpha == other.alpha));
        }

        std::string toString() const
        {
            std::stringstream stream;
            stream << std::hex << std::setw(2) <<
                "r:0x" << int(this->red) << " " <<
                "g:0x" << int(this->green) << " " <<
                "b:0x" << int(this->blue) << " " <<
                "a:0x" << int(this->alpha);
            
            return stream.str();
        }
    };
};

typedef std::vector<std::shared_ptr<GB::RGB>> ColorArray;

class GameBoyInterface {
public:
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

    virtual bool load(const std::string & filename) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void setButton(JoyPadButton button) = 0;
    virtual void clrButton(JoyPadButton button) = 0;
    
    virtual ColorArray getRGB() = 0;
};

#endif /* GAMEBOYINTERFACE_H_ */
