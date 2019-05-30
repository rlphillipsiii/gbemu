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
    std::cout << "Starting the hardware emulation" << std::endl;

    GameBoy console;
    console.start();

    while (1);

    return 0;

	// QGuiApplication app(argc, argv);

#if 0
	qmlRegisterType<Auth>("PassMan.Auth", 1, 0, "Auth");
	qmlRegisterType<CredentialRowModel>("PassMan.CredentialRowModel", 1, 0, "CredentialRowModel");
	qmlRegisterType<Account>();

	//qRegisterMetaType<Account>("Account");

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/qml/viewer.qml")));

	QLoggingCategory::setFilterRules("default.debug=true");
#endif



	//return app.exec();
}
