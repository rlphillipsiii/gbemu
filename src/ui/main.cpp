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

#include "screen.h"
#include "configuration.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    Configuration & config = Configuration::instance();
    config.parse();

    qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

#ifdef DEBUG
    QLoggingCategory::setFilterRules("default.debug=true");
#endif

    return app.exec();
}
