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
HEADERS += disassembly.h
HEADERS += console.h
HEADERS += executiontrace.h

SOURCES += console.cpp
SOURCES += disassembly.cpp
SOURCES += screen.cpp
SOURCES += executiontrace.cpp
SOURCES += main.cpp
