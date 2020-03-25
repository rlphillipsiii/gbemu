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

DEFINES += COMMON_EXPORT

TARGET = utility

PUBLIC_HEADERS += configuration.h
PUBLIC_HEADERS += logging.h
PUBLIC_HEADERS += gbrgb.h

HEADERS += $$PUBLIC_HEADERS

SOURCES += configuration.cpp

publishHeaders($$PUBLIC_HEADERS)
