include(../build.pri)

CONFIG += qt

CONFIG (release, debug|release) {
    CONFIG -= console
}

TEMPLATE = app

QT += core
QT += gui
QT += qml
QT += quick

TARGET = gbc

LIBS += -lutility
LIBS += -lhardware

RESOURCES += resources.qrc

HEADERS += screen.h

SOURCES += screen.cpp
SOURCES += main.cpp
