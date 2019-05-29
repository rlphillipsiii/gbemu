include(../build.pri)

CONFIG += qt
CONFIG -= console

TEMPLATE = app

QT += core
QT += gui
QT += qml
QT += quick

TARGET = gbc

LIBS += -lhardware

#RESOURCES += resources.qrc

HEADERS += cpuclock.h

SOURCES += cpuclock.cpp
SOURCES += main.cpp

publishTarget()