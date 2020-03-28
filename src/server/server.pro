include(../build.pri)

TEMPLATE = app

CONFIG += shared

CONFIG -= qt
CONFIG -= core
CONFIG -= gui

CONFIG (asan) {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS   += -fsanitize=address
}

unix: LIBS += -lpthread

LIBS += -lhardware
LIBS += -lutility

LIBS += -lcrypto
LIBS += -lssl

TARGET = gameserver

PUBLIC_FILES += html/index.html
PUBLIC_FILES += html/gamescreen.js
PUBLIC_FILES += html/404.html
PUBLIC_FILES += html/400.html

PUBLIC_HEADERS += gameserver.h

HEADERS += base64.h
HEADERS += $$PUBLIC_HEADERS

SOURCES += base64.cpp
SOURCES += gameserver.cpp
SOURCES += main.cpp

createDirectory($$join($$list($$PUBLIC_BIN, "html"), "/"))

publishHeaders($$PUBLIC_HEADERS)
publishFiles($$PUBLIC_FILES)
