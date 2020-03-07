include(../build.pri)

TEMPLATE = app

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

LIBS += -lgtest
LIBS += -lhardware

unix: QMAKE_LFLAGS += -pthread

DEFINES += UNIT_TEST

TARGET = unittest

INCLUDEPATH += ../hardware
INCLUDEPATH += ../hardware/memory

HEADERS += ../hardware/processor.h
HEADERS += ../hardware/memorycontroller.h
HEADERS += ../hardware/timermodule.h
HEADERS += ../hardware/interrupt.h
HEADERS += ../hardware/memmap.h
HEADERS += ../hardware/gameboy.h

SOURCES += ../hardware/memorycontroller.cpp
SOURCES += ../hardware/processor.cpp
SOURCES += ../hardware/timermodule.cpp
SOURCES += ../hardware/gameboy.cpp
SOURCES += main.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()
