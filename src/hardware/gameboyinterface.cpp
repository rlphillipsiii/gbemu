/*
 * gameboyinterface.cpp
 *
 *  Created on: Jun 19, 2019
 *      Author: Robert Phillips III
 */

#include <memory>
#include <cassert>

#include "gameboyinterface.h"
#include "gameboy.h"

using std::shared_ptr;

shared_ptr<GameBoyInterface> GameBoyInterface::Instance(GameBoyInterface::ConsoleType type)
{
    switch (type) {
    case GAMEBOY_EMU:
        return shared_ptr<GameBoyInterface>(new GameBoy());

    default: assert(0);
    }

    return nullptr;
}
