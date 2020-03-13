include(../build.pri)

TEMPLATE = app

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

LIBS += -lgtest_main
LIBS += -lgtest

unix: QMAKE_LFLAGS += -pthread

DEFINES += UNIT_TEST

INCLUDEPATH += ../../hardware
INCLUDEPATH += ../../hardware/memory

HEADERS += ../../hardware/memory/memoryregion.h
HEADERS += ../../hardware/memory/mappedio.h
HEADERS += ../../hardware/memory/workingram.h
HEADERS += ../../hardware/memory/readonly.h
HEADERS += ../../hardware/memory/unusable.h
HEADERS += ../../hardware/memory/removable.h
HEADERS += ../../hardware/memory/cgbbios.h
HEADERS += ../../hardware/memory/dmgbios.h

SOURCES += ../../hardware/memory/memoryregion.cpp
SOURCES += ../../hardware/memory/mappedio.cpp
SOURCES += ../../hardware/memory/workingram.cpp
SOURCES += ../../hardware/memory/readonly.cpp
SOURCES += ../../hardware/memory/removable.cpp

HEADERS += ../../hardware/processor.h
HEADERS += ../../hardware/memorycontroller.h
HEADERS += ../../hardware/gpu.h
HEADERS += ../../hardware/timermodule.h
HEADERS += ../../hardware/gameboy.h
HEADERS += ../../hardware/joypad.h
HEADERS += ../../hardware/interrupt.h
HEADERS += ../../hardware/memmap.h
HEADERS += ../../hardware/cartridge.h

SOURCES += ../../hardware/gpu.cpp
SOURCES += ../../hardware/memorycontroller.cpp
SOURCES += ../../hardware/processor.cpp
SOURCES += ../../hardware/opcodes.cpp
SOURCES += ../../hardware/timermodule.cpp
SOURCES += ../../hardware/gameboy.cpp
SOURCES += ../../hardware/joypad.cpp
SOURCES += ../../hardware/cartridge.cpp

SOURCES += main.cpp

publishTarget()
