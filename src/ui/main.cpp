/*
 * main.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>

#include "screen.h"
#include "configuration.h"

int main(int argc, char **argv)
{
    constexpr const char *ORGANIZATION = "rlphillipsiii";

    QGuiApplication app(argc, argv);
    app.setOrganizationName(ORGANIZATION);
    app.setOrganizationDomain(ORGANIZATION);

    // Make sure we parse the configuration before trying to load up the screen
    // the screen is what owns the instance of the gameboy, so the config needs
    // to be parsed before we try to create the screen
    Configuration & config = Configuration::instance();
    config.parse();

    qmlRegisterType<Screen>("GameBoy.Screen", 1, 0, "Screen");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    QObject::connect(&engine, &QQmlApplicationEngine::quit, &QGuiApplication::quit);

#ifdef DEBUG
    QLoggingCategory::setFilterRules("default.debug=true");
#endif

    return app.exec();
}
