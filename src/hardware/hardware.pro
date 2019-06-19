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
PUBLIC_HEADERS += gameboy.h
PUBLIC_HEADERS += interrupt.h
PUBLIC_HEADERS += memmap.h

HEADERS += $$PUBLIC_HEADERS
HEADERS += logging.h

SOURCES += gpu.cpp
SOURCES += memorycontroller.cpp
SOURCES += processor.cpp
SOURCES += gameboy.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()
