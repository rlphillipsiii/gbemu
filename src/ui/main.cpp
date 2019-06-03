/*
 * main.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <iostream>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>

#include "gameboy.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
#if 0
    std::cout << "Starting the hardware emulation" << std::endl;

    GameBoy console;
    console.load("bgbtest.gb");
    
    console.start();
#endif
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    //QLoggingCategory::setFilterRules("default.debug=true");

    return app.exec();
}
