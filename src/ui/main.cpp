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
#include <QVector>

#include <cstdint>

#include "screen.h"

#if 0
#include <fstream>
#include <string>
#include <sstream>

int main(int, char**)
{
    std::ifstream input("tmp.txt");

    std::ofstream output("vram.bin", std::ios::out | std::ios::binary);

    char buffer[2];
    while (input.read(buffer, 2)) {
        std::stringstream str;
        str << buffer[0] << buffer[1];

        uint8_t b = uint8_t(std::stoi(str.str(), 0, 16));
        output.write(reinterpret_cast<char*>(&b), 1);
    }

    return 0;
}
#endif

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

#ifdef DEBUG
    QLoggingCategory::setFilterRules("default.debug=true");
#endif
    
    return app.exec();
}
