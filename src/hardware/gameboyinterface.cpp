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
#include "logging.h"

using std::unique_ptr;

unique_ptr<GameBoyInterface> GameBoyInterface::NULLOPT = nullptr;

unique_ptr<GameBoyInterface> GameBoyInterface::Instance(GameBoyInterface::ConsoleType type)
{
    unique_ptr<GameBoyInterface> ptr;

    switch (type) {
    case GAMEBOY_EMU:
        ptr = unique_ptr<GameBoyInterface>(new GameBoy());
        break;

    default:
        ERROR("Fatal Error: Unknown console type = %d\n", int(type));
        assert(0);
    }

    return ptr;
}
