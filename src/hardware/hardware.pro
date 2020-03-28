include(../build.pri)

TEMPLATE = lib

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

CONFIG (profiling): DEFINES += PROFILING
CONFIG (staticmem): DEFINES += STATIC_MEMORY

CONFIG (asan) {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS   += -fsanitize=address
}

LIBS += -lutility

DEFINES += HARDWARE_EXPORT

INCLUDEPATH += memory
INCLUDEPATH += serial

TARGET = hardware

PUBLIC_HEADERS += gameboyinterface.h

HEADERS += memory/memoryregion.h
HEADERS += memory/mappedio.h
HEADERS += memory/workingram.h
HEADERS += memory/readonly.h
HEADERS += memory/unusable.h
HEADERS += memory/removable.h
HEADERS += memory/cgbbios.h
HEADERS += memory/dmgbios.h

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
HEADERS += consolelink.h
HEADERS += pipelink.h
HEADERS += socketlink.h
HEADERS += $$PUBLIC_HEADERS

SOURCES += gpu.cpp
SOURCES += memorycontroller.cpp
SOURCES += processor.cpp
SOURCES += opcodes.cpp
SOURCES += timermodule.cpp
SOURCES += gameboy.cpp
SOURCES += joypad.cpp
SOURCES += cartridge.cpp
SOURCES += consolelink.cpp
SOURCES += gameboyinterface.cpp

unix: {
    HEADERS += serial/pipelink_linux.h
    HEADERS += serial/socketlink_linux.h

    SOURCES += serial/pipelink_linux.cpp
    SOURCES += serial/socketlink_linux.cpp
}
win32: {
    HEADERS += serial/pipelink_windows.h
    HEADERS += serial/socketlink_windows.h

    SOURCES += serial/pipelink_windows.cpp
    SOURCES += serial/socketlink_windows.cpp
}

publishHeaders($$PUBLIC_HEADERS)
