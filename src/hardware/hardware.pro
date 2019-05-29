include(../build.pri)

TEMPLATE = lib

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

DEFINES += HARDWARE_EXPORT

TARGET = hardware

PUBLIC_HEADERS += processor.h
PUBLIC_HEADERS += memorycontroller.h
PUBLIC_HEADERS += gpu.h

HEADERS += $$PUBLIC_HEADERS

SOURCES += gpu.cpp
SOURCES += memorycontroller.cpp
SOURCES += processor.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()