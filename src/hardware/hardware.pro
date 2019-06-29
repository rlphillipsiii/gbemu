include(../build.pri)

TEMPLATE = lib

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

DEFINES += HARDWARE_EXPORT

TARGET = hardware

PUBLIC_HEADERS += gameboyinterface.h

HEADERS += processor.h
HEADERS += memorycontroller.h
HEADERS += gpu.h
HEADERS += timermodule.h
HEADERS += gameboy.h
HEADERS += interrupt.h
HEADERS += memmap.h
HEADERS += logging.h
HEADERS += $$PUBLIC_HEADERS

SOURCES += gpu.cpp
SOURCES += memorycontroller.cpp
SOURCES += processor.cpp
SOURCES += timermodule.cpp
SOURCES += gameboy.cpp
SOURCES += gameboyinterface.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()
