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

#define LCD_SCREEN_WIDTH  160
#define LCD_SCREEN_HEIGHT 144

namespace GB {
    struct RGB { uint8_t red; uint8_t green; uint8_t blue; uint8_t alpha; };
};

class GameBoyInterface {
public:
    enum ConsoleType {
        GAMEBOY_EMU,
    };

    static std::shared_ptr<GameBoyInterface> Instance(ConsoleType type=GAMEBOY_EMU);

    virtual bool load(const std::string & filename) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual std::vector<GB::RGB> getRGB() = 0;
};

#endif /* GAMEBOYINTERFACE_H_ */
