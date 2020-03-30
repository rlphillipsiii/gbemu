include(../build.pri)

TEMPLATE = lib

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

CONFIG (asan) {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS   += -fsanitize=address
}

TARGET = ipc

win32: LIBS += -lWs2_32

PUBLIC_HEADERS += socketserver.h

HEADERS += $$PUBLIC_HEADERS

SOURCES += socketserver.cpp

publishHeaders($$PUBLIC_HEADERS)

