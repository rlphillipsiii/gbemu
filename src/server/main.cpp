/*
 * main.cpp
 *
 *  Created on: March 26, 2020
 *      Author: Robert Phillips III
 */
#include "configuration.h"
#include "gameserver.h"

int main(int, char **)
{
    // Make sure we parse the configuration before trying to load up the screen
    // the screen is what owns the instance of the gameboy, so the config needs
    // to be parsed before we try to create the screen
    Configuration & config = Configuration::instance();
    config.parse();

    GameServer server;
    server.start();

    return 0;
}
