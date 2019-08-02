include(../build.pri)

TEMPLATE = app

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

DEFINES += UNIT_TEST

TARGET = unittest

INCLUDEPATH += ../hardware

HEADERS += ../hardware/processor.h
HEADERS += ../hardware/memorycontroller.h
HEADERS += ../hardware/timermodule.h
HEADERS += ../hardware/interrupt.h
HEADERS += ../hardware/memmap.h

SOURCES += ../hardware/memorycontroller.cpp
SOURCES += ../hardware/processor.cpp
SOURCES += ../hardware/timermodule.cpp
SOURCES += main.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()
