include(../build.pri)

TEMPLATE = lib

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

CONFIG (profiling): DEFINES += PROFILING
CONFIG (staticmem): DEFINES += STATIC_MEMORY

DEFINES += HARDWARE_EXPORT

INCLUDEPATH += memory

TARGET = hardware

PUBLIC_HEADERS += logging.h
PUBLIC_HEADERS += profiler.h
PUBLIC_HEADERS += gbrgb.h
PUBLIC_HEADERS += gameboyinterface.h
PUBLIC_HEADERS += canvasinterface.h

HEADERS += memory/memoryregion.h
HEADERS += memory/mappedio.h
HEADERS += memory/workingram.h
HEADERS += memory/readonly.h
HEADERS += memory/unusable.h
HEADERS += memory/removable.h

SOURCES += memory/memoryregion.cpp
SOURCES += memory/mappedio.cpp
SOURCES += memory/workingram.cpp
SOURCES += memory/readonly.cpp
SOURCES += memory/removable.cpp

HEADERS += processor.h
HEADERS += memorycontroller.h
HEADERS += gpu.h
HEADERS += timermodule.h
HEADERS += gameboy.h
HEADERS += joypad.h
HEADERS += interrupt.h
HEADERS += memmap.h
HEADERS += cartridge.h
HEADERS += $$PUBLIC_HEADERS

SOURCES += gpu.cpp
SOURCES += memorycontroller.cpp
SOURCES += processor.cpp
SOURCES += timermodule.cpp
SOURCES += gameboy.cpp
SOURCES += joypad.cpp
SOURCES += cartridge.cpp
SOURCES += gameboyinterface.cpp

publishHeaders($$PUBLIC_HEADERS)
publishTarget()
