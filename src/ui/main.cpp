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
#include <fstream>

#include "screen.h"
#include "logging.h"

using std::ifstream;

int main(int argc, char **argv)
{
    if (argc < 2) {
        LOG("USAGE: %s <rom>\n", argv[0]);
        return -1;
    }

    ifstream file(argv[1], std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LOG("Failed to open file \"%s\"\n", argv[1]);
        return -1;
    }
    file.close();
    
    QGuiApplication app(argc, argv);

    qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

#ifdef DEBUG
    QLoggingCategory::setFilterRules("default.debug=true");
#endif
    
    return app.exec();
}
