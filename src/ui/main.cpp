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
    QGuiApplication app(argc, argv);

    qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");
    
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

#ifdef DEBUG
    QLoggingCategory::setFilterRules("default.debug=true");
#endif
    
    return app.exec();
}
